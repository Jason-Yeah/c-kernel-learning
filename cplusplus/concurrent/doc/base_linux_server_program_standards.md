# Linux 服务器程序规范：从 C17 到 C++20

本文按游双《Linux 高性能服务器编程》第二篇第 7 章 7.1～7.6 编排，面向初学者讲解日志、用户信息、进程关系、系统资源限制、工作目录/根目录和后台化。配套 C17/C++20 程序见 [base_linux_server_program_standards](../code/base_linux_server_program_standards/readme.md)。

**原文对照**：已读取目录内书籍 PDF 第 7 章正文，PDF 阅读器页码为 **247～264**。下文按自己的话概括原书组织，并指出现代 Linux 的差异；配套代码为新写的教学示例，不是书中代码复制。

## 章节路线

| 小节 | PDF 页码 | 原书主题 | 本文示例 |
|---|---:|---|---|
| 7.1 | 248～251 | syslogd/rsyslogd、日志级别与掩码 | `log`、C++20 日志 RAII |
| 7.2 | 252～254 | UID/EUID/GID/EGID 与切换用户 | `identity`、受控降权函数 |
| 7.3 | 255～258 | PGID、SID、setsid 与 ps | `process` |
| 7.4 | 259～260 | getrlimit/setrlimit、soft/hard | `limits`，子进程降低限制 |
| 7.5 | 261～262 | getcwd/chdir/chroot | `cwd`、`cwd-child`；chroot 解释 |
| 7.6 | 263～264 | daemon 化步骤与 daemon() | 短生命周期 `daemon-demo` |

## 0. I/O 与进程入门

### 0.1 系统调用、文件描述符与进程

程序运行时成为进程。内核为进程维护地址空间、打开文件表引用、凭据、工作目录和资源限制等状态。文件描述符（fd）是进程用来引用内核 I/O 对象的小整数：文件、管道、socket、终端都可能由 fd 表示。标准输入、标准输出、标准错误通常分别占 fd 0、1、2。

用户程序调用 `open/read/write/syslog` 等接口时处于用户态；需要操作设备、进程凭据或内核日志通道时，通过系统调用/库接口进入内核。系统调用失败时通常返回错误值并设置线程相关的 `errno`。先检查返回值，再读取 `errno`，不要根据旧 errno 判断本次结果。

日志是服务运维 I/O：程序生成事件，日志设施接收并路由，最终写入 journal、文件或远端。日志调用不等价于数据库事务，也未必意味着磁盘已经持久化。资源限制是内核对进程可用资源的约束；工作目录是路径解析的进程状态；UID/GID 与 capabilities 影响访问控制。

### 0.2 C17 语法快速说明

```c
struct rlimit limit = {0};
if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
    perror("getrlimit");
}
```

- `struct rlimit` 是结构体类型；变量 `limit` 包含软限制、硬限制两个字段。
- `{0}` 把字段初始化为零；`limit.rlim_cur` 用点号访问字段。
- `&limit` 是取地址，函数通过指针把结果写回结构体。
- `==` 比较，`=` 赋值；`-1` 是很多 Linux/POSIX 调用的失败返回值。
- `uid_t/pid_t/rlim_t` 是系统头文件定义的类型，打印时应转换为合适整数类型。
- `fork()` 返回两次：父进程拿到子 PID，子进程得到 0，失败为 -1。之后两边执行同一份代码，靠返回值分支区分。
- `errno` 是错误编号；`perror` 将其转换成易读文字。信号处理函数里不能随意调用 `printf`，因为 stdio 不是异步信号安全的。

`_exit()` 立即结束进程，不刷新继承的 C stdio 缓冲；需要保留 `printf` 输出时应先 `fflush(stdout)`。正常应用层结束一般用 `return` 或 `exit`，fork 后子进程在 exec 前错误退出常用 `_exit`。

### 0.3 C++11～20 实践边界

本章大多数接口属于 POSIX/Linux C API。C++ 可以直接调用，但语言标准并不定义 `syslog`、`setuid`、`setrlimit`、`daemon` 等接口。

| 标准 | 本章相关能力 |
|---|---|
| C++11 | RAII、移动语义、`std::unique_ptr`、`std::error_code`；用析构函数管理日志会话、文件和其他资源 |
| C++14 | 通用 lambda 等语言改进；系统接口与 C++11 的边界没有改变 |
| C++17 | `std::filesystem` 查询/表达路径，`std::string_view` 表达不拥有的文本视图 |
| C++20 | `std::format`（库实现需支持）、`std::span`、协程语言机制；没有标准化 Linux daemon、syslog 或凭据 API |

`std::filesystem::current_path()` 是 C++17 标准库查询工作目录的方式，但改变当前目录仍会影响整个进程，并非某个线程私有。多线程程序中一个线程 `chdir` 会影响其他线程的相对路径解析，因此通常应在启动早期设置好，运行后使用绝对路径或目录 fd。`std::format` 的实现支持在不同标准库版本可能不同；日志项目通常还会选用成熟日志库，提供异步写入、结构化字段、轮转和级别控制。

配套 C++20 程序用自定义 `syslog_session` 在构造/析构时 openlog/closelog，展示 RAII；这只是管理资源，不会自动形成现代结构化日志系统。

## 7.1 日志

**原书 PDF 第 248～251 页**：先介绍 Linux 用户日志和内核日志的来源，再讲 syslog API：`openlog` 设置标识、选项和 facility；`syslog` 输出带优先级的消息；`setlogmask` 过滤级别；`closelog` 关闭会话。书中提到 rsyslogd、`/dev/log` 与 `/var/log` 的路径。

### 日志级别与 facility

syslog priority 由 facility（类别）与 severity（严重程度）组成，常用宏有：

| 级别 | 含义（从严重到详细） |
|---|---|
| `LOG_EMERG` | 系统不可用 |
| `LOG_ALERT` | 需要立即处理 |
| `LOG_CRIT` | 严重故障 |
| `LOG_ERR` | 错误 |
| `LOG_WARNING` | 警告 |
| `LOG_NOTICE` | 值得关注的正常事件 |
| `LOG_INFO` | 一般运行信息 |
| `LOG_DEBUG` | 调试细节 |

`LOG_USER`、`LOG_DAEMON` 是 facility，用于日志分类。`openlog(ident, options, facility)` 设置程序标识、PID 等选项和默认 facility。`syslog(priority, format, ...)` 用 printf 风格格式串写日志，外部输入不能直接当格式串，避免格式字符串漏洞。`setlogmask(LOG_UPTO(LOG_INFO))` 允许 INFO 及更严重级别、过滤 DEBUG。`closelog` 释放/关闭相关日志连接。

系统日志服务收到消息后根据配置决定写到哪里。今天可能由 journald 接收，再由 rsyslog 转存，也可能直接由 syslog socket 接收。`/var/log/messages`、`/var/log/syslog` 等路径依发行版和配置变化。用 `journalctl` 或检查当前日志服务配置，不要把某个默认路径当成 Linux 固定标准。`/dev/log` 常是 Unix 域 socket 路径，但实现与容器环境可能不同。

内核日志由内核日志设施生成并进入 ring buffer；当前常用 `dmesg`/journal 查看。应用程序一般不应尝试读取 `/proc/kmsg`；该接口可能受权限限制，并且日志服务通常负责读取与分发。

### 实际工程中的日志

服务日志应包含时间、级别、组件、请求/连接标识和必要上下文；敏感信息应脱敏。记录错误时保存有用 errno/错误码。对高频热路径避免无条件构造昂贵 debug 字符串。stdout/stderr 被 supervisor 接管也很常见；不要自行 daemon 化后又假设终端可见。

C++ 项目可以用 `std::string` 安全构造消息，但 syslog 的格式参数仍要正确匹配类型。现代日志库通常提供编译期格式检查、异步队列、滚动文件和结构化输出；这些是库的能力，不是 C++20 标准自带日志功能。

实验：

```bash
./server_standards log
./server_standards_cpp20
journalctl --since '5 minutes ago' | rg 'server_standards'
```

最后一条依赖本机使用 journald；否则查询系统日志配置指定的目标文件。

## 7.2 用户信息

**原书 PDF 第 252～254 页**：解释真实 UID/GID 与有效 UID/GID，以及 set-user-ID 程序为什么可能以文件所有者权限运行；随后示例展示由 root 启动后切换到普通用户。原书的安全主线仍有价值，但真实部署不能只调用一次 `setuid` 就认为完成了权限隔离。

### real/effective ID 是什么

- **UID/GID**：真实用户和组身份，通常来自启动进程的用户凭据。
- **EUID/EGID**：内核在多种文件访问控制判断中使用的有效凭据。
- **补充组**：用户所属的其他组，也会影响访问权限。

执行带 set-user-ID 位的文件时，exec 后进程的 EUID 可能变为文件所有者；实际行为受挂载选项、内核安全策略和 capabilities 影响。传统 `su` 示例只帮助理解 EUID，不应被当成现代身份切换教程的全部。

查询接口有 `getuid/geteuid/getgid/getegid/getgroups`。设置接口包括 `setgid/setuid` 等，调用失败要检查 errno。Linux 进程本质上有多个凭据集合（real/effective/saved IDs、filesystem IDs、supplementary groups、capabilities）；复杂权限模型应结合 `credentials(7)` 阅读。

### 为什么服务先提权启动、再降权

服务可能需要在启动时读取 root-only 配置、绑定受限端口或准备文件目录，随后应以专门的低权限账户运行。典型顺序是：确认资源准备结束，设置补充组，设置 GID，再设置 UID，验证降权成功，关闭不再需要的特权 fd，并限制 capabilities。调用顺序错误可能失去设置组的权限，或保留不必要权限。

配套 `drop-privileges UID GID` 明确拒绝 root 目标，只在有效 UID 为 root 时运行，并执行 `setgroups → setgid → setuid`。它只是顺序演示，不负责配置服务账户、目录权限、capabilities、namespace、SELinux/AppArmor 或 systemd 安全选项。不要用真实生产进程试验。

```bash
./server_standards identity
id
```

查看当前程序和 shell 的凭据。无需 root 即可学习。

## 7.3 进程间关系

**原书 PDF 第 255～258 页**：讲进程组 PGID、会话 SID、`setpgid/setsid/getsid`，最后用 `ps -o pid,ppid,pgid,sid,comm` 观察父子与作业控制关系。

### 三种 ID

| 名称 | 用途 |
|---|---|
| PID | 唯一标识一个当前进程 |
| PPID | 记录父进程 PID |
| PGID | 把相关进程组织成进程组；shell 用于作业控制和信号投递 |
| SID | 把一个或多个进程组组织成会话，通常与控制终端关联 |

同一进程组中，PGID 通常等于该组首领的 PID；会话首领 PID 通常也等于 SID。进程关系随退出、重新分组和父进程收养改变，所以 PID 不能作为永久身份。

`getpgrp/getpgid` 查询组；`setpgid` 调整进程组，受父子关系、exec 时机和会话边界等约束；`setsid` 创建新会话并脱离控制终端。进程组首领不能直接成功调用 setsid，这也是 daemon 步骤先 fork 再 setsid 的原因之一。

```bash
./server_standards process
ps -o pid,ppid,pgid,sid,tty,stat,comm -p <替换为程序PID>
```

父进程、组、会话关系会影响终端挂断和作业控制信号。systemd 管理服务时也会设置进程组/cgroup，这些是 supervisor 与内核层面的管理关系。

## 7.4 系统资源限制

**原书 PDF 第 259～260 页**：用 `getrlimit/setrlimit` 说明软限制与硬限制，举出 CPU 时间、文件大小、文件描述符等限制，并提到 shell `ulimit` 和系统配置。

```c
struct rlimit {
    rlim_t rlim_cur; /* soft limit */
    rlim_t rlim_max; /* hard limit */
};
```

软限制是当前进程实际适用的上限；硬限制是软限制可提高到的上限。非特权进程通常能降低自己的 hard limit，但不能提高它；soft limit 可在不超过 hard 的范围内调整。`RLIM_INFINITY` 表示无限（或平台定义的无穷值）。限制可能导致 `open/socket/fork/mmap` 失败，也可能触发信号，具体依 resource 类型而定。

常见资源：`RLIMIT_NOFILE` 最大 fd 数，`RLIMIT_CORE` core dump 大小，`RLIMIT_CPU` CPU 时间，`RLIMIT_AS` 虚拟地址空间。`getrlimit` 读取，`setrlimit` 修改当前进程限制。它不是全系统资源的准确预测，也不负责调大机器物理内存。

shell 的 `ulimit -n` 等会影响其启动的子进程；systemd unit 也能设 `LimitNOFILE=` 等。容器、cgroup、PAM 和内核还有其他限制层，程序启动时应观测并报告实际限制，不宜默默修改系统全局配置。

```bash
ulimit -n
./server_standards limits
```

示例只在 fork 出来的子进程降低 NOFILE 软限制，父进程和 shell 不受影响。

## 7.5 改变工作目录和根目录

**原书 PDF 第 261～262 页**：用 `getcwd/chdir` 查询和改变工作目录，接着介绍 `chroot` 改变进程看到的根目录。书中指出 chroot 后应 chdir 到新根，早先打开的 fd 仍可能访问旧树资源。这也是为什么 chroot 本身不是完整安全边界。

### 工作目录

进程的当前工作目录是内核维护的路径解析起点。`getcwd(buf, size)` 获取路径；`chdir(path)` 改变当前工作目录。相对路径 `open("data.txt", ...)` 从当前目录解析，绝对路径从进程根目录解析。

线程共享进程的文件系统上下文，因此一个线程 chdir 会影响同进程其他线程。多线程服务通常避免运行期间 chdir，采用绝对路径或 `openat` 等相对目录 fd 的接口，让路径解析显式依附目录对象。

`std::filesystem::current_path()` 是 C++17 查询/修改 cwd 的接口；它仍改变整个进程状态，不会变成线程局部目录。实验 `cwd-child /tmp` 在子进程 chdir，展示 fork 后 cwd 的变化互不影响。

### 根目录与 chroot

`chroot(path)` 修改进程路径解析所用的根目录，通常需要特权。它不会自动 chdir，也不会关闭此前打开的 fd；进程可能通过已打开目录 fd 或其他资源访问新根之外内容。它也不隔离网络、PID、挂载、系统调用或资源，因此单独 chroot 不是完整容器或安全沙箱。

现代隔离通常依靠容器/namespace、mount 配置、capability 收缩、seccomp、LSM 与服务管理器共同实现。对初学者示例不调用 chroot，避免对宿主系统造成不可逆的路径访问变化。若实验 chroot，应使用一次性容器和专门准备的根文件系统，并理解挂载与权限要求。

## 7.6 服务器程序后台化

**原书 PDF 第 263～264 页**：先列出手工 daemon 化的一般步骤：fork 后父进程退出、设置 umask、setsid、chdir、关闭继承 fd、把标准流接到 `/dev/null`；随后介绍 libc `daemon(nochdir, noclose)` 的简化接口。

### 守护进程的传统步骤

传统 daemon 通常没有控制终端，以后台方式运行。常见手工步骤包括：

1. fork，让启动 shell 得到父进程退出，子进程继续。
2. setsid 创建新会话并脱离控制终端。
3. 可再 fork，避免后续重新获得控制终端。
4. 设置 umask，选择工作目录，通常为 `/` 或明确的服务目录。
5. 关闭继承的无用 fd；处理标准输入、输出、错误，重定向到 `/dev/null` 或日志。
6. 初始化信号、日志、PID/锁文件与服务状态，再进入主循环。

书中的代码展示核心原理，不包含完整健壮服务所需的所有错误处理和 fd 清理。`daemon()` 简化传统过程，但参数 `nochdir/noclose` 行为需读当前 man page；它不帮你实现 PID 文件互斥、日志轮转、优雅停止、重启策略或依赖管理。

### 现代 Linux 服务如何运行

现在通常让 systemd 等 supervisor 在前台启动服务并负责重启、日志收集、资源限制、依赖、权限和 cgroup。程序保持前台运行，收到 SIGTERM 后优雅退出。这样 supervisor 能直接跟踪主进程，日志可写 stdout/stderr 后由 journald 收集，资源限制可声明在 unit 文件中。

后台化不等于高性能；多 fork 也不等于并发。daemon 程序仍要正确处理信号、文件描述符、日志、PID/锁文件和退出状态。不要同时让 systemd 托管又自行 double-fork，除非 unit 类型明确配置匹配。

配套 `daemon-demo 15` 使用 libc daemon，在后台写 syslog 后自行结束，只用于观察进程生命周期。运行后可用 `pgrep -a server_standards` 查看，日志查询方法见 README。

## 4. 配套代码与练习

所有命令、预期现象和代码解释在[配套 README](../code/base_linux_server_program_standards/readme.md)。先运行只读的 `identity/process/limits/cwd/log`，再阅读权限降级与 daemon 示例。用户切换、chroot 和 daemon 化会改变进程权限/环境；先在一次性环境理解影响，再用于服务部署。

复习时试着说明：syslog 为什么不是“写文件 printf”？EUID 如何影响访问权限？setsid 为什么常在 fork 后调用？软限制与硬限制有何差异？多线程程序运行中 chdir 有什么风险？为什么 systemd 服务通常不自行后台化？

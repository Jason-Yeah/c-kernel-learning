# Linux 服务器程序规范：配套实验

配套[主教程](../../doc/base_linux_server_program_standards.md)，依照《Linux 高性能服务器编程》第 7 章 7.1～7.6。示例包含 C17/Linux API 与 C++20 的日志、进程身份和当前目录小程序。

## 文件和编译

- `server_standards.c`：C17 实验：syslog、UID/GID、进程组/会话、资源限制、工作目录、可选权限降级与短生命周期 daemon。
- `server_standards.cpp`：C++20 使用 `std::filesystem` 查询当前目录，并用 RAII 管理 syslog 会话。
- `Makefile`：构建两个程序。

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_linux_server_program_standards
make
```

依赖 Linux、GCC/G++、GNU Make。使用 `-std=c17` 和 `-std=c++20` 选择语言标准。`syslog`、`getuid`、`getrlimit` 等是操作系统接口，不属于 ISO C/C++ 标准库。

## 可以直接运行的安全实验

### 日志

```bash
./server_standards log
./server_standards_cpp20
```

程序使用 `openlog` 建立 syslog 身份/选项，`syslog` 写入 INFO 或 WARNING，C17 版本还用 `setlogmask(LOG_UPTO(LOG_INFO))` 过滤 DEBUG。`closelog` 结束日志会话。日志由系统日志服务路由；不同发行版可能进入 journald 或配置的文件，查找方法见教程。输出“已提交”不保证日志服务器已持久化，更不代表远端已收到。

C++20 版本的 `syslog_session` 是自定义 RAII 类型：构造时 openlog，析构时 closelog，禁止复制以避免会话所有权含糊。它展示资源管理方式，没有包装日志格式或异步写入。

### 用户和组身份

```bash
./server_standards identity
```

显示 PID、真实/有效 UID/GID 和补充组。真实 ID 表示启动身份，effective ID 主要参与文件权限检查。程序只查询，不修改当前身份。

权限降级命令只供理解调用顺序：

```bash
sudo ./server_standards drop-privileges 1000 1000
```

它要求有效 UID 为 root、拒绝目标 root，并按“setgroups → setgid → setuid”顺序永久放弃特权。数字 `1000` 只是常见示例，请先用 `id 用户名` 确认系统上的目标 UID/GID。此操作只影响该程序进程，且通常不可逆；无需 sudo 时该命令会拒绝运行。正式服务器还要审慎保留/清理 capabilities、补充组、已打开特权 fd 和环境信息；不要照抄成通用安全沙箱。

### 进程组、会话

```bash
./server_standards process
ps -o pid,ppid,pgid,sid,tty,stat,comm -p $$,$$
```

程序显示自己的 PID、父 PID、PGID 和 SID。`ps` 命令用来观察 shell 启动的进程关系；用程序的 PID 替换 `$$` 可查看该实例。PID、PGID、SID 是不同概念，终端作业控制通常围绕会话和前台进程组。

### 资源限制

```bash
./server_standards limits
```

输出当前进程的 NOFILE（可打开 fd 数量）、CORE（core 文件大小）、CPU 时间软/硬限制。随后 fork 一个子进程，在子进程里将 NOFILE 软限制最多降低到 64，再尝试打开 `/dev/null`。子进程退出后父进程限制不变。软限制不能高于硬限制；普通进程通常可降低硬限制，却不能自行提高它。系统值随 shell、systemd unit、容器和发行版而异。

### 当前工作目录

```bash
./server_standards cwd
./server_standards cwd-child /tmp
```

第一条显示本进程当前目录。第二条 fork 子进程，只在子进程里调用 `chdir /tmp` 并打印结果；父进程目录保持不变。工作目录是进程属性，影响相对路径解析，不会改变文件系统本身。

### 后台化演示

```bash
./server_standards daemon-demo 15
```

程序调用 libc `daemon(1, 0)` 后脱离终端并在后台运行 15 秒，日志发往 syslog，标准流按 daemon 函数语义重定向到 `/dev/null`。观察时可使用 `pgrep -a server_standards`，然后到日志服务查 `server_standards_daemon_demo`。进程会自行退出；不要用这个小演示承担实际服务。

`daemon()` 的参数 `nochdir=1` 保留当前工作目录，`noclose=0` 关闭并重定向标准输入输出错误。手工 daemon 化还涉及 fork、setsid、umask、chdir、关闭继承 fd、信号处理、PID 文件和启动监督。现代 Linux 服务更推荐由 systemd 等 supervisor 在前台管理生命周期，而不是在业务程序中自行 double-fork。

## 实现细节

- 错误系统调用通过 `perror` 报告，`errno` 仅在失败后读取。
- `getgroups(0, NULL)` 先查询补充组个数，再申请数组读取；数组由 `calloc/free` 管理。
- `getrlimit/setrlimit` 结构中的 `rlim_cur` 是软限制，`rlim_max` 是硬限制；`RLIM_INFINITY` 表示不设数值上限。
- 子进程结束使用 `_exit` 避免重复刷新继承的 stdio 缓冲；需要显示缓冲内容前先 `fflush`。
- 示例不会调用 `chroot`。chroot 需要特权，且单独调用不能构成完整隔离；正确部署还要考虑 chdir、目录权限、已打开 fd、mount namespace/capability/seccomp 等边界。

清理可执行文件：

```bash
make clean
```

# Linux 高级 I/O 函数：从 C17 到 C++20

本文按游双《Linux 高性能服务器编程》第二篇第 6 章 6.1～6.8 的顺序讲解，面向刚接触 Linux I/O 的读者。书中这章讨论管道、描述符复制、向量 I/O、sendfile、内存映射、splice/tee 和 fcntl，重点是这些接口如何连接文件、进程、管道和网络端点。

配套程序位于 [base_advanced_io](../code/base_advanced_io/readme.md)，包含 C17 Linux API 实验和 C++20 RAII/mmap 小例子。示例以 C17 展示 POSIX/Linux 原始接口。C++11～20 的改善重点是对象生命周期和内存表示；它们并没有在标准库中提供 Linux pipe/sendfile 等接口。第 6 章没有要求学并发，这里的 `fork` 例子只用于说明管道两端。

**原文对照**：使用了目录中的书籍 PDF 第 6 章正文（PDF 阅读器第 223～243 页）。以下标出对应页码，概括原书主题并指出适合现代 Linux/C++ 学习时留意的区别。示例为本教程新写，不是书中代码的逐行移植。

## 阅读地图

| 小节 | PDF 页码 | 本文重点 | 实验 |
|---|---:|---|---|
| 6.1 pipe | 223～225 | 单向字节流、端点关闭与 EOF/SIGPIPE | `io_lab pipe` |
| 6.2 dup/dup2 | 226～228 | fd 表和 open file description、重定向 | `io_lab dup` |
| 6.3 readv/writev | 229～232 | 分散读、集中写、短 I/O | `io_lab vectors` |
| 6.4 sendfile | 233～235 | 文件到输出 fd 的内核传输 | `io_lab sendfile FILE` |
| 6.5 mmap/munmap | 236～237 | 虚拟内存映射与页面调入 | `io_lab mmap FILE` |
| 6.6 splice | 238～241 | 至少一端为 pipe 的内核数据移动 | `io_lab splice FILE` |
| 6.7 tee | 242～243 | 不消费源管道数据的复制 | `io_lab tee` |
| 6.8 fcntl | 243 起 | 查询/修改 fd 标志和管道容量 | `io_lab fcntl` |

## 0. 先认识 I/O、内核和 C 语法

### 0.1 什么是输入输出

I/O 是 Input/Output（输入/输出）。程序从文件、终端、管道、socket 读取数据是输入；写入这些对象是输出。Linux 把很多不同对象统一表示为文件描述符（file descriptor，简称 fd）：一个小整数，用来索引当前进程的 fd 表。常见约定是 0 标准输入、1 标准输出、2 标准错误输出。

```text
进程内存（用户态）                     Linux 内核（内核态）
char buffer[]  ← read / write →  fd 表 → 打开文件描述 → 文件/pipe/socket
```

系统调用跨越用户态/内核态边界。`read`、`write` 等把数据从用户缓冲区和内核管理的数据对象之间搬运。内核对象可能代表磁盘文件、匿名管道或网络连接。fd 是索引，不是文件内容，也不是内核对象本身。

### 0.2 复制 fd 时究竟复制了什么

```text
fd 表（进程）             内核打开文件描述（open file description）
  3 ────────────────────┐   文件偏移、状态标志、引用关系
  7（dup(3)）────────────┴──→ 同一对象 → 文件/socket/管道端
```

`dup` 产生另一个 fd，让它与旧 fd 指向同一个打开文件描述。因此读写文件共享当前偏移，`O_NONBLOCK` 这类文件状态标志也共享。fd 自身的 `FD_CLOEXEC` 则属于描述符标志，有单独规则。`fork` 会复制进程 fd 表的引用，子进程与父进程也会引用相同的内核打开文件描述。

这一区别解释了一个常见问题：如果管道写端被 `dup` 或 `fork` 多留了一份，读端即使“看起来已经关闭了原来的写 fd”，也不会看到 EOF；内核仍认为有进程可能写入。

### 0.3 C17 代码里的指针和返回值

```c
int fd[2];
if (pipe(fd) == -1) {
    perror("pipe");
}
```

- `int fd[2]` 是两个整数的数组，`pipe(fd)` 把数组交给函数填写；在函数参数位置，数组会转换为指向第一个元素的指针。
- `==` 是比较，`=` 是赋值。系统调用通常以 `-1` 表示失败，再由 `errno` 说明原因。
- `perror` 输出调用标签和当前 `errno` 对应的错误文字。若中间又调用了其他可能改写 `errno` 的函数，应先保存 `int saved_errno = errno`。
- `void *` 是通用对象指针；`size_t` 是非负的长度类型；`ssize_t` 是有符号字节计数类型，能表达 `-1`。
- `struct stat st = {0};` 把结构体清零初始化；`.` 访问成员；`&st` 取地址。
- `const char *path` 表示指向只读字符的指针，不代表整个文件不可修改。

`read/write/readv/writev/sendfile/splice/tee` 成功时经常可以只处理一部分数据。返回值是这次处理的数量，循环要根据实际进度推进指针/偏移；只检查“不是 -1”就认为全部完成是错误的。

### 0.4 C++11 到 C++20 怎么接入

Linux I/O 接口仍是 C/POSIX 风格函数。C++ 程序可以包含系统头文件后调用它们。现代 C++ 的优势在于可靠地管理资源和表达缓冲区：

| 标准 | 可以用的能力 | 对本章的用途 |
|---|---|---|
| C++11 | RAII、移动语义、`unique_ptr`、`nullptr`、`std::array` | 用对象析构关闭 fd、释放映射 |
| C++14 | 泛型工具进一步完善 | 简化通用封装，不改变内核 API |
| C++17 | `std::byte`、`std::filesystem`、`string_view` | 表示字节或路径；视图不拥有数据 |
| C++20 | `std::span`、`std::endian`、协程语言机制 | 描述连续缓冲区；检查本机字节序；配合外部 I/O 运行时 |

C++20 没有标准化 Linux `pipe/sendfile/mmap` 包装。`std::span` 不拥有内存，不能延长缓冲区寿命；`std::filesystem::path` 也不能代替文件描述符。协程本身不会使阻塞 `read` 变成异步操作。

**RAII（资源获取即初始化）**：把一个资源交给对象独占；对象离开作用域时析构释放。fd 包装类应禁止复制、支持移动，析构时关闭 fd；内存映射包装类析构时 `munmap`。这样函数提前返回或抛异常也不易泄漏。多线程同时操作同一 fd 时仍需要同步与清晰的所有权设计。

## 6.1 pipe 函数

**原书 PDF 第 223～225 页**：先给出 `pipe(fd)` 的两个端点，再解释读空管道会等待、写满会等待；所有写端关闭后读端得到 EOF，所有读端关闭后写入会遇到 SIGPIPE。书中还把管道与 TCP 字节流作比较，并介绍双向的 `socketpair`。

### 单向管道怎么工作

```c
int fd[2];
pipe(fd);
// fd[0] 读；fd[1] 写
```

管道是内核维护的有限容量队列。往写端写入字节，读端按顺序取出这些字节。pipe 默认阻塞：空管道 `read` 等待数据；管道空间不够时 `write` 可能等待。管道是字节流，没有应用层消息边界。

“EOF”并不是管道里特殊放了一个 `-1` 字节。它表示内核确认已经没有任何写端引用；缓冲区已有数据仍可先读完，之后 `read` 返回 0。反过来，当所有读端都关闭，写入会产生 SIGPIPE；默认信号动作通常终止进程。若程序忽略/捕获 SIGPIPE，写调用通常以 `EPIPE` 失败。

```text
父进程 write(fd[1]) → 内核管道缓冲区 → 子进程 read(fd[0])
```

管道通常用于有亲缘关系进程之间的单向通信。要双向通信可用两根管道，或用 Unix 域 `socketpair(AF_UNIX, SOCK_STREAM, ...)` 建立一对双向端点。

管道容量是内核实现与资源限制相关值，不能硬编码成书中的某个固定数字。Linux 可用 `fcntl(F_GETPIPE_SZ)` 查询，在权限和系统限制允许时用 `F_SETPIPE_SZ` 调整。写入量不超过 `PIPE_BUF` 时，多个写者之间有原子性保证；这不表示读者每次会收到一整条写入消息。

### C++ 写法

调用 `pipe2(fd, O_CLOEXEC)` 创建后，马上把两个端点放入不同的 `unique_fd`。父子进程都必须及时关闭各自不用的端点。C++ RAII 不能代替“关闭不需要的副本”，因为未关闭的写端会阻止 EOF。

示例 `io_lab pipe` 用 `fork` 演示父进程写、子进程读，然后双方关闭多余端点。这里使用 Linux `pipe2` 设置 `O_CLOEXEC`，避免 fd 泄漏到 exec 启动的新程序。POSIX 的 `pipe` 本身没有 flags 参数。

## 6.2 dup 函数和 dup2 函数

**原书 PDF 第 226～228 页**：书中用关闭 stdout、再 `dup(connfd)` 的方式说明最小可用 fd 规则，并由此解释 CGI 为什么能把标准输出重定向到客户端 socket。原理正确，但应用代码不能假设 connfd 总是大于 2，也要处理 stdio 缓冲。

```c
int newfd = dup(oldfd);            // 取当前最小可用 fd
int target = dup2(oldfd, wanted);  // 精确替换 wanted
```

如果 `wanted` 已经打开，`dup2` 会先原子地关闭它，再让它指向 `oldfd` 对应的同一打开文件描述。若 `oldfd == wanted`，不关闭它，直接返回。`dup2` 失败返回 -1。比起下面这种分开的写法，`dup2` 避免了两步之间其他线程抢占 `wanted` 的竞态：

```c
close(wanted);
dup(oldfd); // 这时最小 fd 不一定还是 wanted
```

`dup` 后两个 fd 共享打开文件描述的文件偏移和文件状态标志；它们是两个可单独关闭的 fd。

一个重要细节：`dup/dup2` 新 fd 的 close-on-exec 标志默认关闭；Linux 的 `dup3(..., O_CLOEXEC)` 可一次创建带 CLOEXEC 的副本。`dup2` 的替换行为与 `fcntl(F_DUPFD_CLOEXEC)` 也有细节差异。要在多线程程序避免 exec 泄漏，优先使用带 CLOEXEC 的原子创建接口。

示例 `io_lab dup` 保存原 stdout，然后把 fd 1 重定向到 stderr，写出一行，最后恢复。调用 `fflush(stdout)` 是因为 `printf/puts` 有用户态 stdio 缓冲；fd 重定向只改变内核看到的 fd 指向，不会自动搬运 C 标准库缓冲区。

C++ 仍然使用同样的系统调用。不要让两个拥有型 RAII 对象同时认为自己独占同一个 fd；描述符复制要显式表示成 `duplicate()` 或借用关系。

## 6.3 readv 函数和 writev 函数

**原书 PDF 第 229～232 页**：`readv` 把一次读取分散写入多个内存块（scatter/gather 里的 scatter）；`writev` 把多个内存块按顺序集中写出（gather）。书中用 HTTP 头部和文件正文分别保存在不同缓冲区的例子说明可以避免先拼接成一整块。

```c
struct iovec parts[2];
parts[0].iov_base = header;
parts[0].iov_len = header_size;
parts[1].iov_base = body;
parts[1].iov_len = body_size;
ssize_t n = writev(fd, parts, 2);
```

这不是两条独立 write，也不保证 TCP 对端一次 recv 读到相同边界。对于 pipe，`PIPE_BUF` 原子写约束针对整个 writev 请求总长度；超过限制时不具有多写者原子性保证。

`writev` 可能短写。若 n 小于所有 iovec 长度之和，下一次调用必须跳过已完成的整个 iovec，并调整第一个未完成项的 `iov_base/iov_len`。`readv` 返回 n 时，只能使用前 n 个字节；它可能填满前几个 iovec 后只部分填写下一个。非阻塞 fd 还需处理 `EAGAIN` 并等待就绪。

示例用管道进行 writev，再用两个缓冲区 readv，打印实际读取字节。实验数据较少、管道阻塞，因此通常单次完成；真实程序必须按返回值处理短 I/O。现代 C++ 可用 `std::array<iovec, N>` 管理 iovec 数组，用 `std::span` 表达源内存范围，但 `iovec` 本身仍是 POSIX 类型。

## 6.4 sendfile 函数

**原书 PDF 第 233～235 页**：书中说明 `sendfile(out_fd, in_fd, offset, count)` 可在两个 fd 之间传输文件数据，特别适合文件到 socket 的路径，省去应用程序先 read 到用户缓冲区再 send 的往返。书中将其概括为“零拷贝”。这个词需要谨慎理解：它表示避免某些用户态中转复制，不能承诺整个硬件/内核路径完全没有数据复制。

```c
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
```

Linux 上输入端通常要是支持 mmap 的文件，输出端通常是 socket；普通文件发往 stdout/pipe 并非通用兼容组合。接口条件和平台版本应查本机 `man 2 sendfile`。输出 fd 若是 socket，数据仍受对端读取速度、TCP 流控和发送缓冲影响。

若 `offset == NULL`，使用并更新输入文件描述共享的当前偏移；若提供 offset 指针，则从指定偏移传输并更新该变量，文件描述自身偏移不因此改变。成功返回实际传输字节数，可能小于 count；对端关闭、信号、非阻塞或内核限制都可能令传输提前停止。循环处理短传输；n 为 0 通常表示输入到达 EOF。

示例 `io_lab sendfile FILE` 用显式 offset 分块发往 stdout，方便观察字节完全一致。C++ 封装应让输入 fd 用 RAII 管理，并维护 `off_t` 进度。应用协议如 HTTP 仍需要自己发送正确响应头，处理文件变化、权限、范围请求、断连和背压。

## 6.5 mmap 函数和 munmap 函数

**原书 PDF 第 236～237 页**：`mmap` 把文件或匿名内存映射到进程虚拟地址空间，返回可用地址；`munmap` 解除映射。书中介绍读写/执行保护，以及 `MAP_SHARED/MAP_PRIVATE` 对修改传播方式的区别。

```c
void *p = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, offset);
if (p == MAP_FAILED) { /* 注意是 MAP_FAILED，不是 NULL */ }
/* 使用 p[0..length) */
munmap(p, length);
```

CPU 使用虚拟地址，MMU 页表将其映射到物理页或文件页缓存。映射成功通常不意味着整个文件已立刻读入 RAM；首次访问页面时可能触发缺页异常，由内核调入数据。之后页面也可能被回收。mmap 是内存访问方式，不保证物理上零拷贝，也不意味着访问不会阻塞。

- `MAP_SHARED`：对映射的修改可被其他共享映射观察，文件修改何时持久化还涉及 `msync/fsync` 与文件系统语义。
- `MAP_PRIVATE`：写时复制，修改是私有的，不作为文件修改写回。
- `offset` 必须满足页对齐要求；长度为 0 时不能建立普通文件映射。
- 文件被其他进程截短后，访问映射超出新文件末尾可能产生 `SIGBUS`。
- 文件描述符在成功 mmap 后可关闭；映射仍存在，直到 munmap 或进程退出。

示例只读映射常规文件，并逐字节写到 stdout。C++ 可用 `mapped_region` 风格的 RAII 类封装，但析构时须使用原始地址和长度调用 `munmap`。不要把 mmap 得到的字节区域直接 reinterpret 成任意复杂 C++ 对象：对象生命周期、对齐、编码和文件格式都要满足要求。

## 6.6 splice 函数

**原书 PDF 第 238～241 页**：书中强调 `splice` 在两个 fd 间移动数据时至少一个必须是管道，再把 socket 到 pipe、pipe 到 socket 串起来做回射服务器。管道作为内核传递通道，能避免数据绕到用户缓冲区；并不意味着所有文件系统、设备和协议组合都支持。

```c
ssize_t splice(int fd_in, loff_t *off_in,
               int fd_out, loff_t *off_out,
               size_t len, unsigned int flags);
```

至少一个端点要是 pipe。输入或输出端是 pipe 时，相应 offset 必须为 NULL；普通文件可用 offset 指定位置，但 seekable 对象上的偏移规则要看 man page。非阻塞时可能返回 `EAGAIN`；成功也可能只移动一部分。

示例将普通文件 splice 到 pipe，再从 pipe splice 到 stdout。每次先读取最多 65536 字节进入 pipe，然后循环把这一批排空，避免 pipe 满时继续写入导致死锁。真实非阻塞代理还需分别维护输入、pipe 缓存、输出的进度，并用 epoll/poll 驱动。

`SPLICE_F_MOVE` 是提示，不能据此声称内核一定把物理页所有权移动。`SPLICE_F_MORE` 对输出端表达后续还有数据的提示；flags 是优化提示，不改变协议正确性。

## 6.7 tee 函数

**原书 PDF 第 242～243 页**：Linux `tee` 在两个 pipe fd 之间复制数据，不消费源 pipe 的数据；随后可分别从两根 pipe 读出同一份内容。这和 shell 命令 `tee`（复制标准输入到文件和终端）是不同概念。原书示例用 Linux tee 程序展示拼接方式。

```c
ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags);
```

`fd_in` 和 `fd_out` 都必须是 pipe。成功返回复制的字节数，不改变源 pipe 的读位置；返回 0 表示当前没有可复制数据。阻塞/非阻塞行为及短复制仍要处理。若只反复 tee 而从不消费源 pipe，缓冲区终将占满。

示例 `io_lab tee` 从标准输入取不超过 4096 字节到第一根 pipe，用 tee 复制到第二根，再分别 splice 到 stdout 和 stderr。输入需短于 pipe 容量；实验刻意缩小范围以便看清“不消耗源内容”。在事件循环服务里必须设计容量、EOF 和背压，不能把这段当无限流实现。

## 6.8 fcntl 函数

**原书 PDF 第 243 页起**：本章前文已用 `fcntl` 设置文件描述符属性和查询/修改管道容量。`fcntl` 的第三个参数因 command 不同而有不同类型，书中特别展示 `F_GETPIPE_SZ/F_SETPIPE_SZ`。理解 command 对应的参数类型比背诵单一函数原型更重要。

```c
int fcntl(int fd, int cmd, ...);
```

常见 command 分几类：

| 命令 | 操作对象 | 典型用途 |
|---|---|---|
| `F_GETFD / F_SETFD` | fd 自身标志 | `FD_CLOEXEC` |
| `F_GETFL / F_SETFL` | 共享 open file description 状态标志 | `O_NONBLOCK`、`O_APPEND` |
| `F_DUPFD / F_DUPFD_CLOEXEC` | 描述符复制 | 找到不小于指定数值的可用 fd |
| `F_GETPIPE_SZ / F_SETPIPE_SZ` | Linux pipe 容量 | 查询/请求调整容量 |

调用 `F_SETFL` 时通常先 `F_GETFL`，保留已有状态位后再增删目标位：

```c
int old = fcntl(fd, F_GETFL);
fcntl(fd, F_SETFL, old | O_NONBLOCK);
```

不要把 `FD_CLOEXEC` 传给 `F_SETFL`：它属于 descriptor flags，应配 `F_GETFD/F_SETFD`。反过来，`O_NONBLOCK` 属于 open file description 状态，dup 出来的 fd 共享此状态。需要避免“顺手覆盖其他标志”的写法。

示例用 `pipe2(O_NONBLOCK | O_CLOEXEC)`，然后 `F_GETFL` 读取标志，验证空管道非阻塞 read 返回 `EAGAIN`，并查询可用时的 pipe 容量。对被共享的 fd 修改 `O_NONBLOCK` 会影响其他引用者，必须在组件设计中明确这一点。fcntl 是可变参数接口，命令不同参数类型不同，避免把指针/整数传错。

## 1. 数据路径：这些 API 为什么可能快

普通 read/write 的概念路径：

```text
文件页缓存/内核对象 → 用户缓冲区 → 内核 socket 发送缓冲区 → 网络
```

`writev` 能省去应用层把多块数据拼成一块的复制；`sendfile` 可避免数据先复制到用户缓冲区；`splice/tee` 通过管道在内核对象间传递引用或数据页；`mmap` 让进程把文件页缓存映射进虚拟地址空间。

“零拷贝”通常是工程简称，意思是减少特定路径上的 CPU 复制和用户态往返。页缓存填充、校验、加密、文件系统、驱动、网卡 DMA 等环节仍可能复制或转换。更少复制也不一定让整个服务更快：系统调用数、缺页、缓存命中、拥塞、磁盘等待和应用调度都可能成为瓶颈。先定义基准，再比较吞吐、延迟、CPU 和内存。

## 2. 从 C 资源管理迁移到 C++

C 示例采用显式 `close/munmap`。实际项目中应让每条错误路径释放已创建资源；资源越来越多时可用 `goto cleanup` 集中清理，这是 C 中常见风格。

C++ 可定义不可复制、可移动的 `unique_fd`，析构调用 close；另一个 `mapped_file` 保存映射指针和长度，析构调用 munmap。它们应是两个资源类型，因为 fd 和映射的生命周期独立。`std::unique_ptr<T, Deleter>` 适合拥有单个分配对象，不一定适合带长度、地址和映射语义的所有资源。

文件描述符 API 使用原始指针和长度时，C++20 可以用 `std::span<const std::byte>` 作为调用侧接口，再把 `.data()` 与 `.size()` 传给系统函数。调用期间 span 所指对象必须存活；异步任务更要把缓冲区所有权保存在任务状态中。`std::expected` 是 C++23，不应说成 C++20 标准能力；C++20 项目可用错误码、`std::error_code`、异常或项目自有结果类型。

## 3. 编译和练习

详细命令、每个实验预期现象和完整源码解析见[配套 README](../code/base_advanced_io/readme.md)。先运行 pipe 和 dup，再做 vectors，最后比较 mmap、sendfile、splice 的接口限制。

建议思考：

1. 为什么子进程忘记关闭 pipe 写端会让父进程读不到 EOF？
2. `dup` 后两个 fd 关闭顺序任意吗？文件偏移是独立的吗？
3. `writev` 返回 10 但总长度为 30，下一次从哪里继续？
4. mmap 成功是否表示文件所有数据已经进 RAM？文件被截短后会怎样？
5. `splice` 为什么需要 pipe，`tee` 复制后源数据为什么还在？
6. 为什么 `O_NONBLOCK` 与 `FD_CLOEXEC` 通过不同 fcntl 命令管理？

把答案说清楚，你就掌握了第六章的核心：fd 指向内核对象，系统调用在对象间传递字节，而 C/C++ 程序负责进度、生命周期和错误恢复。

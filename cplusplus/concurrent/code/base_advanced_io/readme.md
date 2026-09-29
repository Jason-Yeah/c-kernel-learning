# 高级 I/O 函数学习程序（C17 / C++20）

配套[主教程](../../doc/base_advanced_io.md)，按《Linux 高性能服务器编程》第 6 章 6.1～6.8 编排。本目录中的程序用短实验观察 fd、内核缓冲和数据传递；它们不是生产服务器或性能基准。

## 文件说明

| 文件 | 语言 | 实验 |
|---|---|---|
| `io_lab.c` | C17 | pipe/fork、dup2 重定向、readv/writev、mmap/munmap、sendfile、splice、tee、fcntl |
| `io_cpp20.cpp` | C++20 | 用 RAII 管理 fd 和只读 mmap，以 `std::span<std::byte>` 连接映射内存和 write |
| `Makefile` | make | 编译两个独立程序 |

## 编译

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_advanced_io
make
```

需要 Linux、GCC/G++ 和 GNU Make。`-std=c17` 与 `-std=c++20` 分别启用语言标准；`-Wall -Wextra -Wpedantic` 打开常用警告。pipe2、splice、tee、sendfile 和 F_GETPIPE_SZ 等是 Linux 接口；换操作系统时需改写或检测能力。源码声明 `_GNU_SOURCE` 以获得 GNU/Linux 扩展声明。

## io_lab.c 各实验

### 6.1 `./io_lab pipe`

父进程向管道写入一行，子进程从读端读出并打印。

`pipe2` 返回 `p[0]`（读端）和 `p[1]`（写端）。fork 后两边都继承两个端点，所以子进程必须关闭写端，父进程必须关闭读端。父进程写完关闭最后一个写端，子进程才会在读完缓冲区后见到 EOF。程序用 `waitpid` 等子进程退出。`_exit` 用来避免 fork 子进程重复刷新从父进程复制来的 stdio 缓冲区。

### 6.2 `./io_lab dup`

保存 stdout，再用 `dup2(2, 1)` 将标准输出指向标准错误，打印一行后恢复 stdout。你会看到第一行走 stderr，第二行回到原 stdout。

`dup` 返回当前最小可用 fd；`dup2(oldfd, targetfd)` 精确替换目标 fd。两者建立到同一 open file description 的引用。`puts` 属于 stdio 层，所以重定向前 `fflush(stdout)`，避免缓冲内容在恢复后写到错误目标。

### 6.3 `./io_lab vectors`

`writev` 从两个独立字符串块集中写入 pipe，`readv` 将字节分散到两个数组，再按实际返回字节数输出。iovec 保存地址与长度，不拥有缓冲区。

实验数据较小、fd 阻塞，通常一次读写完成；生产代码必须处理短读写。TCP 是字节流，不保留 writev 形成的“块边界”。

### 6.4～6.6 文件路径对照

准备文件：

```bash
printf '文件内容示例\n' > /tmp/advanced_io_input.txt
```

执行并观察三者 stdout：

```bash
./io_lab mmap /tmp/advanced_io_input.txt
./io_lab sendfile /tmp/advanced_io_input.txt
./io_lab splice /tmp/advanced_io_input.txt
./io_cpp20 /tmp/advanced_io_input.txt
```

- `mmap` 将文件映入虚拟地址空间，程序访问映射页，再用 write 输出。
- `sendfile` 把文件 fd 内容直接送到 stdout。Linux 常见实现主要面向 socket 输出；如果你的内核不支持该 fd 组合，会报错。真实网络服务用已连接 socket 做 out fd。
- `splice` 把文件内容送入 pipe，再从 pipe 送到 stdout；循环正确排空每批数据。
- `io_cpp20` 展示相同 mmap 读取思路，但文件 fd 和映射各有 RAII 所有者。输入文件应在程序运行时保持不被截短。

这些输出路径用于观察 API 参数限制，不代表三者都是同一种“零拷贝”实现。`sendfile` 和 `splice` 的适用 fd 组合因内核/文件系统而异，需查当前机器 `man 2 sendfile`、`man 2 splice`。示例不宣称性能一定更快。

### 6.7 `printf '复制两份\n' | ./io_lab tee`

程序把标准输入的一小段送进 pipe A，用 `tee(A读端, B写端, ...)` 复制到 pipe B。之后 B 的数据经 splice 写入 stderr，A 的原数据经 splice 写入 stdout。终端上可能看到两份内容（stdout 与 stderr 的显示次序由终端调度决定）。将两路分别重定向可检查：

```bash
printf 'sample\n' | ./io_lab tee > /tmp/tee-out.txt 2> /tmp/tee-err.txt
cmp /tmp/tee-out.txt /tmp/tee-err.txt
```

实验只处理一段不超过管道容量的输入，便于说明 tee 不消费源 pipe；它不是无限长度的复制工具。无限流需要循环处理短 splice/tee、管道背压和 EOF。

### 6.8 `./io_lab fcntl`

创建非阻塞 pipe，使用 `F_GETFL` 读取状态标志；空管道 read 返回 `EAGAIN/EWOULDBLOCK`，这表示“现在没数据”，不同于 EOF（返回 0）。若 Linux 支持 `F_GETPIPE_SZ`，程序也会打印此 pipe 的容量。容量受内核配置与资源限制影响，输出值并非固定答案。

`fcntl(fd, F_GETFD/F_SETFD, ...)` 操作 fd 自身标志（如 FD_CLOEXEC）；`F_GETFL/F_SETFL` 操作共享 open file description 状态（如 O_NONBLOCK）。dup 后两个 fd 共享 O_NONBLOCK 的状态变化。修改标志时先读取原值，再保留其他位。

## io_cpp20.cpp 看点

`unique_fd` 禁止复制、允许移动，离开作用域自动 close。`read_mapping` 保存 mmap 地址与长度，析构时 munmap。两个类分开是因为文件 fd 可以先关闭，而映射仍能继续存在。

`std::span<const std::byte>` 表示一段连续字节的地址和长度，不分配、不拥有数据。`write_all` 每次成功后用 `subspan(n)` 跳过已写部分，因此兼容短写。span 所指内存必须在使用期间有效；异步操作还需延长底层缓冲区生命周期。

`system_fail` 用 `std::system_error` 保存 errno；最外层 catch 输出诊断。C++20 语言标准本身没有 mmap/socket 的标准包装，这里的类是教程自定义 RAII 封装。

## 安全边界与练习

程序只读取命令行指定文件；不要对来历不明的特殊设备文件运行 mmap/sendfile/splice 实验。`io_cpp20` 和 mmap 实验假设文件在映射期间不被截短，否则访问文件末尾之后的映射区可能收到 SIGBUS。

可尝试修改 pipe 示例，故意不关闭父进程或子进程的写端，观察 EOF 为什么消失；把 vectors 缓冲区改小观察返回长度；把 tee 输入扩展到超过管道容量，分析为什么单次实验会失败或阻塞。完成这些后，再阅读主教程对非阻塞 I/O 和背压的解释。

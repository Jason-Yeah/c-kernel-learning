# Linux I/O 复用配套实验（C17 / C++20）

对应[主教程](../../doc/base_IO%20multiplexing.md)，按游双《Linux 高性能服务器编程》第 9 章 9.1～9.8 编排。程序只绑定本机回环地址，避免意外对外开放端口。

## 程序一览

| 文件 | 语言/标准 | 展示内容 |
|---|---|---|
| `select_poll.c` | C17 | select 和 poll 对比；一个循环同时等待 TCP listener、UDP socket 和多个 TCP 客户，TCP/UDP 回显 |
| `epoll_tcp_udp.cpp` | C++20 + Linux | epoll LT/ET、非阻塞 accept/read/write、同端口 TCP+UDP、fd RAII |
| `nonblocking_connect.c` | C17 | EINPROGRESS、poll 等待完成、SO_ERROR 判定成功/失败、连接超时 |
| `chat_poll.c` | C17 | poll 聊天室客户端同时等待 stdin/socket；服务器 poll 多个客户端并广播 |
| `Makefile` | make | 编译上述四个可执行程序 |

这些程序是课堂实验，不是生产服务器。`chat_poll` 的广播 send 是阻塞式，慢客户端可能拖慢全体；回显程序没有完整限流/超时/服务治理；TCP 数据仍需要应用层 framing。

## 编译

需要 Linux、GCC/G++、GNU Make；C++ 示例要求编译器支持 C++20。

```bash
cd "/home/jason/study/ck_study/cplusplus/concurrent/code/base_IO multiplexing"
make
```

`-std=c17`/`-std=c++20` 选择语言标准；`-Wall -Wextra -Wpedantic` 开启常用警告。select/poll 是 POSIX 接口，epoll 是 Linux 专有接口。所有程序已用 C17/C++20 编译。

## 实验 A：对比 select 和 poll

分别启动：

```bash
./select_poll select 9100
./select_poll poll 9100
```

每次运行选一种模式，另开终端用 `nc 127.0.0.1 9100` 测 TCP；用支持 UDP 的 `nc -u 127.0.0.1 9100` 测 UDP。服务会把收到的内容原样回显。Ctrl+C 停止。

程序同时创建 TCP stream socket 与 UDP datagram socket 并绑定相同端口，说明传输协议不同可共享端口号。select 版每次循环重建 `fd_set`，因为 select 返回时会修改集合；`nfds` 取最大 fd 加 1，并在 `FD_SET` 前检查 fd 是否小于 `FD_SETSIZE`。poll 版复用 `pollfd[]`，关注 `revents`，关闭的客户槽位设为 fd -1。

程序维护最多 128 个 TCP 客户。事件循环在一次返回中依次处理 listener、UDP 和已连接客户 fd，因此它并不是多个线程并行运行。某个 fd 读就绪后 `recv` 可能得到 EOF，此时关闭客户 fd。

## 实验 B：epoll LT/ET 和 TCP+UDP

```bash
./epoll_tcp_udp 9101 lt
./epoll_tcp_udp 9101 et
```

使用 `nc 127.0.0.1 9101` 或 UDP nc 客户端验证回显。LT 和 ET 都可运行；观察不同模式下事件通知习惯。ET 模式下 listener、UDP 和 accepted socket 均为非阻塞；代码对 accept/recv/send 循环处理直到 EAGAIN，写不完的数据保存在客户输出字符串中，并注册 EPOLLOUT。

TCP 和 UDP 分别创建 socket，绑定同一个地址和端口，加入同一个 epoll fd。TCP listener 可读意味着有新连接；TCP 客户可读意味着字节流数据；UDP fd 可读意味着至少有一份数据报。内核按 fd 返回事件，应用再按角色分派。

代码使用 `epoll_create1(EPOLL_CLOEXEC)`，防止 exec 子进程意外继承 epoll fd。`epoll_ctl` 的 ADD 注册事件，MOD 根据待发数据更新 EPOLLOUT，DEL 移除关闭连接。C++ `Fd` 类禁止复制、支持移动，析构 close。

## 实验 C：非阻塞连接

先在一个终端启动 TCP 服务，例如 `./select_poll poll 9102`。另一个终端运行：

```bash
./nonblocking_connect 127.0.0.1 9102 3000
```

连接成功输出 `connected`。测试拒绝连接可选一个没有监听的端口，例如：

```bash
./nonblocking_connect 127.0.0.1 9103 3000
```

过程是 `socket(...SOCK_NONBLOCK...)` 后 connect。若返回 EINPROGRESS，poll 等待 POLLOUT；就绪后 `getsockopt(SO_ERROR)` 才是最终结果。poll 超时不会让连接自动取消，示例通过 close fd 终止本次尝试。

本程序接受数字 IPv4，不做域名解析；它展示一个连接。真正的 Happy Eyeballs/多地址并发连接需要维护多个候选 fd、共享一个总截止时间，并在某一连接成功后取消其余连接。

## 实验 D：聊天室

启动服务端：

```bash
./chat_poll server 9104
```

分别在两个终端启动客户端：

```bash
./chat_poll client 127.0.0.1 9104
```

任意客户端输入文字并回车，其他客户端会收到。客户端 poll 的两个 fd 分别是标准输入和服务端 socket；服务端 poll 管理 listener 和客户 socket。Ctrl+D 关闭客户端发送方向。

为突出 I/O 复用，本例服务器收到一段数据就同步广播，并直接在事件循环里 send。生产服务器不应对慢接收者做阻塞 send；应为每个连接维护待发队列、限制队列大小、监听可写事件并实现背压。

## 对源码初学者的导读

### `select_poll.c`

- `fd_set` 是固定大小的位集合；`FD_ZERO/FD_SET/FD_ISSET` 是设置和读取位的宏。
- `maxfd + 1` 是 select 的第一个参数；nfds 是 fd 编号范围，不是监听 fd 数目。
- select 每次返回后集合只剩就绪项，因此必须在 while 下一轮重新设置。
- poll 的每项 `{fd, events, revents}` 把描述符和事件绑定。内核写 `revents`，程序每次检查实际事件。
- `recv == 0` 表示 TCP 对端结束发送；UDP 的零长度数据报则是合法消息。

### `epoll_tcp_udp.cpp`

- `epoll_event.data.fd` 将返回事件关联回 fd；本例用 map 查找 TCP 客户状态。
- LT 允许事件持续保持；ET 要一次排空数据至 EAGAIN，所以把监听 socket 与接受的 socket 都设为非阻塞。
- send 可能短写；`Client.sent` 是已发送偏移。剩余字节挂起时加 EPOLLOUT，发完后移除，避免 socket 常年可写导致空转。
- 收到 TCP EOF 时停止关注 EPOLLIN，但仍排空已接收数据的回显，再关闭。
- UDP 使用 `recvfrom/sendto` 保留数据报边界。这里每次 datagram 都单独回显，不把 UDP 当 TCP 字节流。

### `nonblocking_connect.c`

- 非阻塞 connect 可能立即成功、以 EINPROGRESS 表示进行中，或立即失败。
- POLLOUT 代表连接过程已结束，不保证成功；`SO_ERROR == 0` 才表示成功。
- `poll` 返回正数也要检查 `revents`，并通过 SO_ERROR 取得最终错误。

### `chat_poll.c`

- 客户端用同一个 poll 等终端输入和网络消息，不需要一个线程阻塞在键盘、另一个线程阻塞在 socket。
- 服务端把每个 fd 放进 pollfd 数组，读取后遍历其他客户端发送。
- poll 事件集合允许 fd 为 -1 表示暂时跳过槽位。真正的长连接服务还需动态容器、连接对象、写队列和最大连接数。

## 复习

1. 为什么 select 返回后要重建 fd_set？
2. `poll` 的 `events` 与 `revents` 分别由谁写？
3. 为什么 epoll ET 必须非阻塞并读到 EAGAIN？
4. socket 可写是否等同于非阻塞 connect 成功？为什么要 SO_ERROR？
5. 为什么同端口 TCP 和 UDP 需要两个 socket？
6. I/O 复用有没有自动提供并行？慢客户端会怎样影响当前聊天服务器？

清理可执行文件：

```bash
make clean
```

# 高性能服务器程序框架示例

对应[主教程第 8 章](../../doc/base_server_application_framework.md)。这是教学用 Linux TCP 行协议服务器，展示：

- C/S 服务端 socket 生命周期；
- Linux `epoll` 水平触发事件循环；
- 主线程拥有连接 I/O，工作线程池处理业务；
- `eventfd` 将工作线程完成结果通知回事件循环；
- `std::mutex`/`std::condition_variable` 的生产者消费者任务队列；
- 每连接 Reading → Processing → Writing → Reading 的有限状态机；
- 非阻塞 socket、短读/短写、`EAGAIN`、TCP 半关闭与资源 RAII。

本程序不是 HTTP 服务器，也不是生产级服务器。它没有 TLS、认证、超时、最大连接数、队列上限、速率限制、配置系统或完整优雅关停。任务队列没有设置上限，长时间过载会积压内存；请用于本机学习。

## 编译与启动

依赖 Linux、G++ 支持 C++20、pthread 和 Make：

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_server_application_framework
make
./framework_server 9000 4
```

参数是监听端口和工作线程数；端口也可以给 `0`，由内核分配端口，启动行会打印实际端口。服务器仅绑定 `127.0.0.1`，默认四个 worker。按 Ctrl+C 或发送 SIGTERM 结束。

另一个终端用 Python 标准库测试：

```bash
python3 - <<'PY'
import socket
with socket.create_connection(('127.0.0.1', 9000)) as s:
    s.sendall(b'hello server\n')
    print(s.recv(1024).decode(), end='')
PY
```

预期回复：

```text
OK HELLO SERVER
```

单条连接逐次发送的每一行都会得到对应响应。可以故意分段发送：先发 `b'hel'`，再发 `b'lo\n'`，服务器仍在接收状态累积数据，只有换行到齐才提交业务。TCP 可能合并或拆分 send，程序不能依赖发送边界。

用 `ss -lntp | rg ':9000'` 观察监听端口。使用端口 0 时请将 9000 换成程序打印的实际端口。`strace -f -e epoll_wait,epoll_ctl,accept4,recvfrom,sendto,eventfd2 ./framework_server 9000 2` 可观察系统调用（不同 libc 可能把 recv/send 显示为 recvfrom/sendto）。

## 代码结构详解

### `unique_fd`

只允许一个对象拥有 fd；禁止复制，析构关闭。这样异常退出某层函数时，epoll/eventfd/listener 仍会释放。连接对象也拥有自己的 socket fd。`epoll_ctl(DEL)` 后从连接表移除，再由共享对象生命周期关闭连接。

### `Connection` 与状态机

连接数据只由主 I/O 线程修改：

- `input` 保存尚未组成完整行的输入；
- `output` 和 `sent` 保存待发响应及短写偏移；
- `peer_eof` 记录客户端是否关闭了发送方向；
- `state` 控制当前是否接收、等待业务、发送或关闭。

每个连接同时最多一个在途请求。读取到换行后，主线程把这一行从输入缓存移出，状态改为 Processing 并将 job 投进线程池。此时 EPOLLIN 暂停，避免客户端继续推入无界请求。完成结果回来后转入 Writing；响应全写完才重新读取并处理缓存中的下一行。

换行分帧比直接假设一次 recv 是一条请求可靠。输入行超过 4096 字节会关闭连接。EOF 时未结束的最后一行被丢弃；已经提交的请求仍会完成并发送响应。

### epoll 主循环

listener 和每个 client socket 均为非阻塞、CLOEXEC。主线程等待 epoll 事件：

1. listener 可读时循环 `accept4`，直到 `EAGAIN`，为每个连接创建状态对象并注册 EPOLLIN。
2. client 可读时循环 `recv`，直到 `EAGAIN`，把得到字节追加到输入缓冲并尝试解析一行。
3. client 可写时循环 `send`，直到响应发完或遇到 `EAGAIN`。发送使用 `MSG_NOSIGNAL`，同时进程忽略 SIGPIPE。
4. eventfd 可读时清空计数器，取出 worker 结果队列，将响应放入连接并注册 EPOLLOUT。

这里采用 epoll LT（水平触发）。代码仍循环读写到暂时无进展，避免就绪期间遗漏已有数据。`epoll_wait` 自身阻塞等待事件，而 client fd 是非阻塞的，两者并不矛盾。

### 线程池和结果通知

任务队列由 mutex 保护；condition_variable 让没有任务的 worker 睡眠。worker 取出一行后执行大写转换，把 Completion 放入完成队列，再向 eventfd 写入计数。eventfd 是 Linux 内核提供的计数 fd，能够被 epoll 监听，因此工作线程可以唤醒主事件循环。

worker 不直接改 Connection 的 `state/input/output`，避免和 epoll 线程并发读写同一对象。任务和完成结果持有 `shared_ptr<Connection>`，确保连接对象在异步任务期间存活；主线程仍检查该连接是否仍在活动连接表中，若已关闭就丢弃迟到结果。

线程池析构会停止接收新任务、唤醒 worker 并等待其结束。生产服务还需考虑队列有界、关停期限、任务取消、业务阻塞和worker异常处理。

## API 关键点

- `socket(... SOCK_NONBLOCK | SOCK_CLOEXEC ...)` 创建非阻塞监听 socket。
- `accept4(..., SOCK_NONBLOCK | SOCK_CLOEXEC)` 为连接 fd 原子设置标志。
- `epoll_create1(EPOLL_CLOEXEC)` 创建 epoll 实例，`epoll_ctl` 注册/修改兴趣事件。
- `eventfd(EFD_NONBLOCK | EFD_CLOEXEC)` 用作线程完成通知。
- `std::condition_variable::wait(lock, predicate)` 会在检查条件和睡眠之间正确配合 mutex，减少丢失唤醒的风险。
- `std::shared_ptr` 共享对象生命周期，不保证对象成员自动线程安全；本例依靠“worker 只读任务字符串、I/O 线程独占连接状态”来避免数据竞争。
- `std::thread` 的析构前必须 join；本例线程池析构负责通知和 join。
- `std::from_chars`（C++17）解析命令行整数，不依赖区域设置，也不抛异常。

## 如何和书中模型对应

- C/S：本程序是监听并服务客户端请求的一端。
- 框架四单元：epoll/I/O 主线程、工作逻辑、（没有）存储、job/completion 队列。
- I/O 模型：非阻塞 socket + epoll readiness，同步 recv/send。
- 事件处理：Reactor 风格；I/O 在线程循环，业务在线程池。业务拿到已读完的一行，和“同步 I/O 模拟 Proactor”有相似交接，但内核并未替程序完成 socket 异步读写。
- 并发模式：I/O 线程 + 同步 worker pool；队列由锁保护，属于半同步/半异步思想的一个教学变体。
- 有限状态机：每连接显式记录 reading/processing/writing/closed。
- 性能项：线程池复用线程，按行拥有字符串减少复杂共享；示例还存在队列锁和字符串复制，不能当零拷贝实现或 benchmark。

删除生成程序：

```bash
make clean
```

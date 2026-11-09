# Linux I/O 复用：从 C17 到 C++20

本文按游双《Linux 高性能服务器编程》第二篇第 9 章 9.1～9.8 编排，面向第一次学习 Linux I/O 的读者。章节覆盖 `select`、`poll`、`epoll`、非阻塞 connect、聊天室、TCP+UDP 同端口服务和 xinetd。

配套程序位于 [base_IO multiplexing](../code/base_IO%20multiplexing/readme.md)。已核对同目录的书籍 PDF 第 9 章正文，PDF 阅读器页码 **312～359**；下文标出各节页码并用自己的话概括书中内容。示例是新写的，供本机练习，不是原书代码的复制。

“C11-C20”按 **C11/C17** 和 **C++11/14/17/20** 两条标准理解。C++20 不等于 C20。`select/poll/epoll` 都是操作系统接口，不属于 ISO C/C++ 标准库；现代 C++ 的主要帮助是资源管理、类型安全和组织异步状态。

## 目录

- [0. 先理解什么是 I/O 复用](#0-先理解什么是-io-复用)
- [9.1 select](#91-select)
- [9.2 poll](#92-poll)
- [9.3 epoll](#93-epoll)
- [9.4 三组接口比较](#94-三组接口比较)
- [9.5 非阻塞 connect](#95-非阻塞-connect)
- [9.6 用 I/O 复用做聊天室](#96-用-io-复用做聊天室)
- [9.7 同时处理 TCP 与 UDP](#97-同时处理-tcp-与-udp)
- [9.8 xinetd 与现代 socket activation](#98-xinetd-与现代-socket-activation)
- [10. C++11～20 的使用方式](#10-c11～20-的使用方式)

## 书中章节与 PDF 页码

| 原书小节 | PDF 页码 | 原书重点 | 配套示例 |
|---|---:|---|---|
| 9.1 | 313～319 | select API、就绪条件、异常集合与 OOB | `select_poll select` |
| 9.2 | 320～322 | pollfd 数组和超时 | `select_poll poll`、`chat_poll` |
| 9.3 | 323～336 | epoll 内核表、wait、LT/ET、ONESHOT | `epoll_tcp_udp lt/et` |
| 9.4 | 337～339 | 接口、容量、工作模式、实现复杂度比较 | 本文比较表 |
| 9.5 | 340～343 | 非阻塞 connect 和 SO_ERROR | `nonblocking_connect` |
| 9.6 | 344～350 | poll 聊天客户端/服务器 | `chat_poll` |
| 9.7 | 351～354 | 一个 epoll 同时处理 TCP、UDP | `epoll_tcp_udp`、`select_poll` |
| 9.8 | 355～359 | xinetd 配置与启动子服务流程 | 文档说明；不改系统服务配置 |

## 0. 先理解什么是 I/O 复用

### 0.1 问题：一个程序如何等很多 fd

文件描述符（fd）是进程用来引用 I/O 对象的小整数。socket、管道、终端和文件都可能有 fd。假设服务器同时有监听 socket 和 10,000 个客户端连接：如果逐个阻塞 `recv`，程序可能卡在第一个暂时没数据的客户上；如果对每个客户都创建线程，又要付出大量线程栈和调度成本。

I/O 复用让程序把一组 fd 和感兴趣的事件交给内核，等待“其中哪些现在就绪”。比如服务器可以同时等：监听 socket 可读（新连接），连接 socket 可读（客户数据），连接 socket 可写（发送缓冲区腾出空间）。

```text
应用注册关注的 fd 和事件
          │ select / poll / epoll_ctl
          ▼
内核等待任一 fd 就绪 ──事件返回──> 应用处理对应 fd
```

### 0.2 I/O 复用会阻塞，但仍然可以服务很多连接

`select/poll/epoll_wait` 本身常常阻塞，直到有事件或超时。这不代表服务器只能等一个连接：一次等待可覆盖多个 fd，哪个先就绪都能唤醒程序。连接 socket 通常设为非阻塞，这样事件后读取到当前没有更多数据时会返回 `EAGAIN`，事件循环就可以继续处理其他连接。

I/O 复用**不是线程，也不会自动并发执行回调**。一个事件循环线程收到多个就绪事件后，仍按代码顺序处理。需要并行计算时，再配合工作线程/进程池，或把业务拆到异步任务。

“就绪”只表示现在执行某类 I/O 有机会取得进展，不表示完整请求已经到齐，也不表示一次读写能完成所有数据。TCP 是字节流：一次 `recv` 可能只得到半条消息，必须由应用协议定义分帧方式。

### 0.3 C17 与 C++ 的基本 API 写法

```c
fd_set reads;
FD_ZERO(&reads);
FD_SET(fd, &reads);
int n = select(fd + 1, &reads, NULL, NULL, NULL);
```

`&reads` 是取变量地址，让 `select` 能读写这个集合。`NULL` 表示没有注册相应集合或不设置超时。`== -1` 表示失败，之后才看 `errno`。返回正数是就绪 fd 的总数，不是数据字节数。

在 C++ 中系统函数也在全局命名空间，常写 `::select(...)` 表示明确调用这个全局 API。`std::vector<pollfd>`、RAII fd 类和 `std::chrono` 可以改善调用周边的生命周期和可读性，但 epoll 本身仍需传原始 fd 与事件结构。

## 9.1 select

**原书 PDF 第 313～319 页**：书中按 `nfds`、三个 fd 集合和 `timeval` 解释 select，然后介绍 socket 何时可读/可写，最后用异常 fd 集合检测 TCP 带外数据。

```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

- `readfds`：等待可读事件，例如连接有数据、对端关闭、监听 socket 新连接到来。
- `writefds`：等待可写事件，例如发送缓冲区有空间，或非阻塞 connect 已完成（成功或失败都可能让它就绪）。不要一直监听普通连接的可写事件，因为大多数时候 socket 都可写，会让循环忙转。
- `exceptfds`：网络 socket 上主要用于带外（urgent/OOB）数据这类异常条件，不是泛指所有错误。
- `nfds`：所有被监听 fd 的最大值加 1。fd 从 0 开始编号。
- `timeout == NULL`：一直等；全零 timeval：立即探测；非零：最多等指定时间。不同系统对返回后的 timeout 修改规则存在差异，重复调用前重新初始化。

`fd_set` 容量受 `FD_SETSIZE` 限制，Linux/glibc 常见默认值是 1024；这是编译期位集合限制，不是系统 fd 总数限制。传入 fd 超出 fd_set 支持范围会导致未定义行为，所以必须先检查范围。每次调用前都要 `FD_ZERO`/`FD_SET` 重建集合，因为内核返回时会把它改成“已经就绪的集合”。

就绪判断有实用含义：socket 可读可能是有数据，也可能是 EOF（对端关闭写端），还可能是错误；只有调用 `recv` 看返回值才能区分。可写也不等于整条响应都能写完，仍需处理短写。

书中 9.1.3 用 `exceptfds` 和 `recv(MSG_OOB)` 处理带外数据。现代应用通常避免把 TCP OOB 当作普通优先消息机制；若学习此接口，要区分普通可读与紧急数据标记，并参考第 5 章带外数据说明。

**适合何时使用**：fd 数量较少、兼容性重要、已有 select 代码。新 Linux 服务管理大量连接时一般选 epoll 或现成事件库。select 的 `FD_SETSIZE` 限制和每轮扫描成本是主要约束。

## 9.2 poll

**原书 PDF 第 320～322 页**：poll 与 select 都等待一组 fd，但用 `pollfd` 数组描述“fd + 注册事件 + 返回事件”，不受 fd_set 位图这种固定编号上限约束；书中还提到 GNU 的 `POLLRDHUP`。

```c
struct pollfd {
    int fd;       /* 负数表示忽略 */
    short events; /* 输入：想监听的事件 */
    short revents;/* 输出：实际发生的事件 */
};
int poll(struct pollfd *fds, nfds_t count, int timeout_ms);
```

常见事件有 `POLLIN`、`POLLOUT`、`POLLERR`、`POLLHUP`、`POLLNVAL`；`POLLRDHUP` 可帮助发现对端关闭连接的写方向（需 GNU/Linux 定义）。`POLLHUP/POLLERR` 即使没在 events 请求，也可能出现在 `revents`，所以检查时不能只看 `POLLIN`。

poll 不修改 events，只把实际事件写到 revents，因此数组可复用；关闭某连接后把对应 `fd` 设为 `-1` 可以暂时忽略它。timeout 单位是毫秒，`-1` 永久等待，0 立即返回。

poll 没有 select 的 `FD_SETSIZE` 位图容量限制，但每次等待仍要把数组交给内核，内核和应用仍可能扫描所有注册项。大量连接里只有少部分活跃时，这种 O(n) 扫描会有成本。

配套 `chat_poll` 是书中聊天室思路的简化版本：服务器 poll 监听监听 socket 和所有客户 socket，客户发来的字节广播给其他人；客户端 poll 同时监听标准输入和网络 socket。代码为了初学清晰采用同步广播写，慢客户端可能拖住服务器，因此不能用于生产。

## 9.3 epoll 系列系统调用

**原书 PDF 第 323～336 页**：epoll 用独立 epoll fd 标识内核事件表，`epoll_ctl` 添加/修改/删除兴趣事件，`epoll_wait` 只返回就绪事件；再介绍 LT、ET 和 `EPOLLONESHOT`。

### 9.3.1 创建与注册事件

```c
int epollfd = epoll_create1(EPOLL_CLOEXEC);
epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
```

`epoll_create1` 返回另一个 fd，代表内核事件实例。`event.events` 是监听事件，`event.data` 是用户关联数据（常放 fd 或连接对象指针）。不要同时把 `data.fd` 和 `data.ptr` 当两个独立字段使用，它们是 union 共用空间。用指针时必须保证对象生命周期覆盖内核可能返回该指针的时间。

`epoll_wait` 将就绪事件复制到用户提供的数组，只需遍历返回的 n 个事件，而不是每轮扫描全部注册 fd。现代代码优先用 `epoll_create1(EPOLL_CLOEXEC)`，而不是旧式 `epoll_create`；`size` 参数在新 Linux 已不再表示事件容量。

### 9.3.2 epoll_wait

```c
int n = epoll_wait(epollfd, events, maxevents, timeout_ms);
```

返回 n 个就绪事件；0 是超时；-1 看 errno，信号中断常见 `EINTR`。`maxevents` 是本次最多取回多少个事件。若活跃事件超过数组容量，下轮等待还能取剩余事件，不代表其余 fd 永久丢失。

### 9.3.3 LT 与 ET

| 模式 | 触发条件 | 初学时的处理方式 |
|---|---|---|
| LT（电平触发，默认） | 条件仍成立就会继续通知 | 可先读一部分，下轮通常还会再通知 |
| ET（边沿触发） | 从未就绪变为就绪等状态变化时通知 | fd 必须非阻塞；循环读/写，直到 `EAGAIN` |

ET 中如果只读一次便返回事件循环，数据仍留在接收缓冲区，但状态未发生新的边沿，程序可能一直等不到下一次通知。因此 ET 配套非阻塞 fd，并把数据读到 `EAGAIN`。监听 socket 使用 ET 时也要循环 `accept4` 到 `EAGAIN`。

ET 并不天然比 LT 快。它减少重复通知，但代码更容易漏事件，缓冲管理更复杂。先用 LT 做正确，再通过测量判断是否需要 ET。配套 `epoll_tcp_udp` 可通过 `lt/et` 参数切换，代码在两种模式下都排空读缓冲。

`EPOLLONESHOT` 会在事件通知后暂时禁用该 fd 的注册事件，适合多个线程共享一个 epoll、希望单次只交给一个线程处理连接的场景。处理线程完成后必须 `EPOLL_CTL_MOD` 重新武装事件，否则 fd 后续就绪也不会再通知。若由单个 I/O 线程独占连接状态，通常不需要 ONESHOT。

### 9.3.4 哪些 fd 可 epoll

epoll 主要用于 Linux 支持 poll/epoll 的 fd 类型，例如 socket、pipe、eventfd。普通文件通常总被认为就绪，不能指望 epoll 等待磁盘文件的“数据到达”。不同 fd 类型的事件语义不同，查对应 man page。

## 9.4 三组接口比较

**原书 PDF 第 337～339 页**：从事件集合、fd 数量、工作模式、内核实现路径比较 select/poll/epoll。书中结论倾向大量连接时选 epoll，但也指出活跃连接比例高时 epoll 未必更快。

| 特征 | select | poll | epoll |
|---|---|---|---|
| 注册方式 | 每次传入/返回 fd_set | 每次传入 pollfd 数组 | `epoll_ctl` 注册到内核事件实例 |
| fd 数量限制 | fd_set 通常固定容量 | 无固定 fd_set 上限 | 无 fd_set 位图上限，受系统资源限制 |
| 每轮成本 | 复制集合并扫描 | 复制数组并扫描 | 通常仅返回就绪项，注册表留在内核 |
| 触发模式 | LT 类似语义 | LT 类似语义 | LT、ET、ONESHOT |
| 可移植性 | POSIX，较高 | POSIX，较高 | Linux 专有 |
| 合适场景 | 少量 fd/兼容代码 | 中小规模或跨平台 | Linux 高连接数事件驱动服务 |

不要把复杂度简化成“epoll 永远 O(1)”：内核仍需处理事件和连接，活跃 fd 很多时事件回调/数据传递成本同样增加；epoll_ctl、锁、缓存命中、用户事件处理也都有成本。少量 fd 时 poll 可能更简单甚至更快。以目标负载基准测量。

## 9.5 非阻塞 connect

**原书 PDF 第 340～343 页**：通过非阻塞 socket 发起连接，connect 返回 `EINPROGRESS` 后用 select/poll 等待可写，再通过 `getsockopt(SO_ERROR)` 判断连接最终成功还是失败。此方式可并行等待多个连接尝试或为连接建立设置超时。

```text
socket(O_NONBLOCK) → connect
  ├─ 返回 0：立即成功
  ├─ 返回 -1, errno=EINPROGRESS：等待 POLLOUT/EPOLLOUT
  └─ 其他错误：立即失败
等待写就绪 → getsockopt(SOL_SOCKET, SO_ERROR)
  ├─ 0：连接成功
  └─ 非 0：该值就是连接失败原因
```

**为什么不能只看“可写”**：socket 连接完成或失败后都可能变为可写。必须读取 `SO_ERROR`。connect 等待也要有超时，避免网络黑洞导致无限期占用连接槽。

示例 `nonblocking_connect IPv4 PORT TIMEOUT_MS` 使用 poll。若 `poll` 被信号打断会重试；超时返回错误；就绪后读取 SO_ERROR。示例只支持数字 IPv4 地址，实际客户端通常先用 getaddrinfo 取得 IPv4/IPv6 候选，再分别连接。

现代 C++ 可用 `std::chrono::steady_clock` 管理总期限，配合事件循环定时器；不能在每次 poll 重试时重新从头计算完整 timeout，否则总时限会被信号/多次等待延长。

## 9.6 用 I/O 复用做聊天室

**原书 PDF 第 344～350 页**：聊天室客户端同时等待键盘输入和服务器 socket；服务器 poll 监听新连接及所有用户输入，并广播给其他客户端。这样一个线程可以管理多个连接。

客户端有两个输入源：标准输入和网络 fd。若只阻塞在 `recv`，用户输入无法及时发送；只阻塞在 `read(stdin)` 又无法即时显示其他人的消息。poll 同时监听二者便解决了等待问题。

服务器把新连接加入 poll 集合；任一客户发来数据后，遍历其他在线客户并发送。真实聊天室还要加用户身份、消息分帧、写缓冲队列、慢客户端处理、最大连接数和超时。

运行示例：

```bash
./chat_poll server 9010
```

在两个终端分别运行：

```bash
./chat_poll client 127.0.0.1 9010
```

任一客户端输入文本并回车，其他连接会收到。Ctrl+D 关闭当前客户端的发送方向。示例服务端广播使用阻塞 send，慢客户端会拖住事件循环；这是为了突出 poll 的事件集合，不是可扩展聊天室实现。

## 9.7 同时处理 TCP 与 UDP

**原书 PDF 第 351～354 页**：同一 IP/端口可以分别建立 TCP 和 UDP socket，因为传输协议不同；将监听 TCP socket 与 UDP socket 都加入同一 epoll 实例，分别处理 accept/连接数据和无连接 datagram。

必须创建两个 socket：TCP 用 `SOCK_STREAM` 并 `listen`，UDP 用 `SOCK_DGRAM` 并直接 `recvfrom`。它们绑定同一个端口值时，内核按协议号区分，不冲突。TCP 数据有连接状态，是字节流；UDP 每次接收一份数据报并得到来源地址，零长度数据报合法。

配套 `epoll_tcp_udp PORT [lt|et]` 实现同端口 TCP 和 UDP 回显；`select_poll` 也有 select/poll 版本。示例监听回环地址，只供本机访问。

```bash
./epoll_tcp_udp 9011 et
```

TCP 客户端可用 `nc 127.0.0.1 9011`；UDP 测试可使用支持 UDP 的 `nc -u 127.0.0.1 9011`。不同 nc 实现的选项略有区别。连接建立后发一行，服务器会回显相同字节。

## 9.8 xinetd 与现代 socket activation

**原书 PDF 第 355～359 页**：介绍 xinetd 作为超级服务，读取统一配置和每项子服务配置，监听多个服务端口。收到连接后，内部服务由 xinetd 直接处理；外部服务则 fork/exec 子进程，并把已连接 socket 复制到标准输入输出等 fd。`wait` 设置影响父服务是继续接连接还是等待子进程。

这套模型展示了“一个进程统一监听、按需启动服务”的思想，但具体发行版今天可能没有安装/启用 xinetd。systemd 的 socket activation 是现代 Linux 常见方案：unit 文件声明监听 socket，systemd 持有监听端点，服务启动时通过 fd 接管。它可以延迟启动、集中管理重启和依赖，但服务程序必须支持接收继承的监听 fd。

不要为了学习随意编辑 `/etc/xinetd.conf` 或 systemd 系统级 unit。配套文档只讲概念，不会改动系统服务；当前项目代码可直接在本机运行。

## 10. C++11～20 的使用方式

| C++ 标准 | 与 I/O 复用相关的能力 | 说明 |
|---|---|---|
| C++11 | RAII、移动语义、`std::thread`、mutex/condition_variable | 封装 fd 生命周期，组织事件循环和工作线程 |
| C++14 | 泛型 lambda、`make_unique` 改进 | 方便回调和资源对象创建；无新标准 socket API |
| C++17 | `std::optional`、`string_view`、`from_chars`、`filesystem` | 表达可选结果、解析数据；view 不拥有缓冲区 |
| C++20 | `std::span`、`jthread`、stop token、协程语法 | 可描述缓冲区/线程停止；协程需要事件运行时 |

标准 C++ 库目前没有 epoll/select/poll 的跨平台网络事件循环。可以自己封装，也可以使用 Boost.Asio、libevent、libuv 等库；这些库不是 C++20 标准的一部分。封装系统 fd 时要规定谁拥有、谁关闭，异步回调持有的数据要活到回调结束。

C++ 的 RAII 适合封装 epoll fd、socket fd 和事件注册；`std::vector<epoll_event>` 管理返回事件数组。`std::span<std::byte>` 可表达一段缓冲区，但它不负责分配或延长内存寿命。协程可改变代码的控制流写法，不改变 TCP 分帧、短读写、背压或取消语义。

完整编译命令、示例说明和运行方式见[配套 README](../code/base_IO%20multiplexing/readme.md)。

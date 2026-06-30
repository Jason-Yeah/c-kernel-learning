# Boost 库、.hpp 文件与 ASIO 编程 — 从零开始

> **目标读者**：学过 C/C++ 基础语法，用过标准库（`<iostream>`、`<vector>` 等），但**没接触过 Boost、没听过 .hpp、对网络编程只知道 socket 但没写过异步代码**。本文带你从"这是什么"一直追问到"内核里到底怎么跑的"。
>
> **阅读路径**：Boost 是什么 → .hpp 是什么 → ASIO 是什么 → 同步 vs 异步 → 内核收发包的真实流程 → ASIO 的 Proactor 模型 → 一个完整的 ASIO 例子

---

## 目录

1. [Boost 是什么](#1-boost-是什么)
2. [.hpp 文件是什么](#2-hpp-文件是什么)
3. [ASIO 是什么](#3-asio-是什么)
4. [从 C socket 到 ASIO：API 对照与 endpoints.cpp 逐行拆解](#4-从-c-socket-到-asioapi-对照与-endpointscpp-逐行拆解)
5. [同步 I/O vs 异步 I/O](#5-同步-io-vs-异步-io)
6. [内核到底怎么收发数据的](#6-内核到底怎么收发数据的)
7. [阻塞与非阻塞——内核视角](#7-阻塞与非阻塞内核视角)
8. [I/O 模型演进：从多进程到 Proactor](#8-io-模型演-进从多进程到-proactor)
9. [ASIO 的 Proactor 模型](#9-asio-的-proactor-模型)
10. [第一个 ASIO 例子——简单的 TCP 服务器](#10-第一个-asio-例子简单的-tcp-服务器)
11. [第一个 ASIO 例子——客户端](#11-第一个-asio-例子客户端)
12. [ASIO 的底层——不用 epoll 也行？](#12-asio-的底层不用-epoll-也行)
13. [总结](#13-总结)

---

## 1. Boost 是什么

### 1.1 一句话

**Boost 是一个 C++ 第三方库的"大礼包"**，里面包含了几十个功能各异的库，从正则表达式到图算法、从日期时间到网络编程。它由 C++ 标准委员会成员维护，很多 Boost 里的功能后来直接进了 C++ 标准（比如智能指针 `shared_ptr`、线程库 `std::thread`）。

```
Boost 大礼包里有：

├── Boost.Asio        ← 异步网络编程（本文主角）
├── Boost.Beast       ← HTTP / WebSocket（基于 Asio）
├── Boost.Filesystem  ← 文件系统操作
├── Boost.Regex       ← 正则表达式
├── Boost.Thread      ← 线程库
├── Boost.SmartPtr    ← 智能指针（后来进了 C++11）
├── Boost.Test        ← 单元测试
├── …… 总共 160+ 个库
```

### 1.2 为什么要学 Boost

三个理由：

1. **C++ 标准的"试验田"**——很多 Boost 库后来直接变成了 C++ 标准的一部分。学了 Boost.Asio，你就理解了 C++ 网络编程的基石。
2. **跨平台**——Boost 在 Windows、Linux、macOS 上都能跑，内部帮你处理了系统差异。
3. **生产级**——大型 C++ 项目（比如 Bitcoin Core、cppreference.com）都用 Boost，不是玩具。

### 1.3 Boost 的包管理

Boost 很大（编译后几百 MB），通常不需要全装。你只需要装 `libboost-dev`（基础头文件）+ 你实际用到的库（如 `libboost-asio-dev`）。在 Ubuntu 上：

```bash
sudo apt install libboost-dev libboost-asio-dev
```

**头文件路径**：安装后头文件在 `/usr/include/boost/` 下，你的编译器会自动找到它们（因为 `/usr/include` 在默认搜索路径里）。

---

## 2. .hpp 文件是什么

### 2.1 不是 .h 而是 .hpp，为什么？

你见过 `#include <iostream>`、`#include <vector>`，它们的扩展名是**没有后缀**或者 `.h`。那 `.hpp` 是什么？

**`.hpp` 就是 C++ 的头文件**，跟 `.h` 一样都是头文件。区别只在于命名习惯：

| 扩展名 | 通常含义 | 历史原因 |
|--------|---------|---------|
| `.h` | C 语言的头文件，或者 C/C++ 通用头 | C 语言年代留下的传统 |
| `.hpp` | **纯 C++ 的头文件** | 显式告诉别人"这是 C++，不是 C" |
| `.hxx` | 同样表示 C++ 头文件 | 较少用 |
| （无后缀） | C++ 标准库头文件 | `<iostream>`、`<vector>` 等 |

**关键点**：编译器**不 care 扩展名**。`.h`、`.hpp`、`.hxx` 对编译器来说都一样，区别只存在于人的习惯中。

### 2.2 那为什么 Boost 全部用 .hpp？

因为 Boost 是一个**纯 C++** 项目，里面大量用到了 C++ 特有的东西（模板、命名空间、异常等），用 `.hpp` 后缀是**在文件层面就声明"这是 C++，别用 C 编译器编译"**。

打开 Boost 的头文件看一眼：

```cpp
// /usr/include/boost/asio.hpp  (实际内容节选)
#ifndef BOOST_ASIO_HPP
#define BOOST_ASIO_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
// ... 大量模板代码

#endif
```

看到了吗？`#ifndef` / `#define` / `#endif` 就是头文件保护（header guard），跟 `.h` 一模一样。只是后缀不同。

### 2.3 Boost 全是头文件？那编译怎么那么慢？

这个问题问到点子上了。Boost 的库分为两种：

| 类型 | 含义 | 例子 |
|------|------|------|
| **Header-only** | 只有 `.hpp`，不需要编译 `.cpp`，`#include` 就能用 | Boost.Asio（大部分）、Boost.SmartPtr、Boost.TypeTraits |
| **需要编译的** | 除了 `.hpp`，还要编译对应的 `.cpp` 并链接 `.so`/`.a` | Boost.Filesystem、Boost.Regex、Boost.Thread |

**Header-only 的库编译慢**——因为每次 `#include` 相当于把整个头文件复制进你的 `.cpp` 里，编译器每次都要重新解析几千行模板代码。但好处是**部署简单**，你的程序自带一切，不需要额外链接动态库。

Boost.Asio 大部分是 header-only，但为了性能，建议链接 `libboost_asio`（包含一些编译好的 .o 文件，可以减少模板实例化的体积）。

---

## 3. ASIO 是什么

### 3.1 全称

**ASIO = Asynchronous I/O**（异步输入/输出）。

它是一个 C++ 库，让你**用同一套代码写跨平台的网络程序**——在 Linux 上它用 epoll，在 Windows 上它用 IOCP，在 macOS 上它用 kqueue。你不需要知道这些细节，ASIO 替你选了。

### 3.2 ASIO 解决什么问题

如果不用 ASIO，写一个能处理几千个客户端连接的 Linux 服务器，你需要：

1. 学会 `epoll_create()`、`epoll_ctl()`、`epoll_wait()`
2. 自己维护一个事件循环
3. 处理边缘触发（ET）和水平触发（LT）的区别
4. 处理非阻塞 I/O 的 `EAGAIN` 错误
5. 把读到的数据拼成完整的包（粘包问题）
6. 换个平台（比如 Windows），全部重写

**用 ASIO 的话**，上面这些它全包了：

```cpp
// ASIO 版本——不需要知道 epoll、kqueue、IOCP
asio::io_context io;
asio::ip::tcp::acceptor acceptor(io, ...);
acceptor.async_accept([](error_code ec, socket s) {
    // 有新连接来了，在这里处理
});
io.run();  // 启动事件循环
```

### 3.3 ASIO = 独立库，也是 Boost 的一部分

ASIO 有两个版本：

| 版本 | 命名空间 | 头文件 |
|------|---------|--------|
| **Boost.Asio** | `boost::asio::` | `#include <boost/asio.hpp>` |
| **Standalone Asio**（独立版） | `asio::` | `#include <asio.hpp>` |

独立版更轻量，但内容几乎一样。本文用 Boost.Asio，因为它安装方便。

---

## 4. 从 C socket 到 ASIO：API 对照与 endpoints.cpp 逐行拆解

> **这段为什么放在这里？** 你手边有一个 `endpoints.cpp` 文件，里面全是 ASIO 的基础零件展示（endpoint、socket、acceptor、resolver），但你看着一脸懵。这不怪你——你还不清楚这些 C++ 类型对应 C socket 里的哪个老朋友。这一节手把手把每个"零件"拆开，每个函数都拿 C 风格的 socket 代码对照着讲。

### 4.1 C socket API → ASIO 对照总表

先把 C 语言传统 socket 编程的每个步骤和 ASIO 的对应关系列出来，这是本文**最重要的一张表**：

| # | C socket 操作 | 对应的 ASIO 代码 | endpoints.cpp 中的函数 |
|---|--------------|-----------------|----------------------|
| 1 | `int fd = socket(AF_INET, SOCK_STREAM, 0)` | `tcp::socket sock(ioc);` | `create_tcp_socket()` |
| 2 | `struct sockaddr_in addr;` | `tcp::endpoint ep(ip, port);` | `client_end_point()` |
| 3 | `addr.sin_addr.s_addr = INADDR_ANY` | `ip::address_v4::any()` | `server_end_point()` |
| 4 | `inet_aton("127.0.0.1", &addr)` | `ip::make_address("127.4.8.1")` | `client_end_point()` |
| 5 | `bind(fd, (sockaddr*)&addr, len)` | `acceptor.bind(ep)` | `bind_acceptor_socket()` |
| 6 | `listen(fd, backlog)` | `acceptor.listen(backlog)` | `accept_new_connection()` |
| 7 | `int conn = accept(fd, NULL, NULL)` | `acceptor.accept(sock)` | `accept_new_connection()` |
| 8 | `connect(fd, (sockaddr*)&addr, len)` | `sock.connect(ep)` | `connect_to_end()` |
| 9 | `gethostbyname("example.com")` | `resolver.resolve(host, port)` | `dns_connect_to_end()` |
| 10 | `close(fd)` | socket 析构函数自动 `close()` | RAII（不用手动调用） |

**核心思想**：C socket 操作的是一个**整数**（`int fd`），ASIO 操作的是一个**C++ 对象**（`tcp::socket`、`tcp::acceptor`）。对象在创建时分配资源，在析构时自动释放——这叫 **RAII（Resource Acquisition Is Initialization）**。

### 4.2 endpoint——IP 地址 + 端口号的"打包盒"

**C 语言的做法**：

```c
// C 风格：把 IP 和端口塞进一个结构体
struct sockaddr_in addr;
addr.sin_family = AF_INET;             // IPv4
addr.sin_port = htons(9190);           // 端口（网络字节序）
inet_aton("127.4.8.1", &addr.sin_addr); // IP 地址转二进制
```

每次都要声明结构体、手动设置地址族、手动转字节序、手动转换 IP 字符串。繁琐且容易忘。

**ASIO 的做法**：

```cpp
// ASIO：endpoint = IP + 端口，一步搞定
boost::asio::ip::address ip = boost::asio::ip::make_address("127.4.8.1");
boost::asio::ip::tcp::endpoint ep(ip, 9190);
```

**`endpoint`（端点）就是一个 "IP 地址 + 端口号" 的打包盒**。在 C socket 里你需要自己组装 `sockaddr_in`，在 ASIO 里一个 `tcp::endpoint` 全包了。

再看 `endpoints.cpp` 里的 `client_end_point()`：

```cpp
int client_end_point()
{
    std::string raw_ip_addr = "127.4.8.1";
    unsigned short port = 9190;

    // ===== 解析 IP 字符串 =====
    // C 语言 inet_aton("127.4.8.1", &addr.sin_addr)
    // ASIO 版本：make_address 把 "127.4.8.1" 转换成 ASIO 内部的地址对象
    boost::system::error_code ec;                              // ← 错误信息放这里（稍后细讲）
    boost::asio::ip::address ip_addr =
        boost::asio::ip::make_address(raw_ip_addr, ec);

    // 检查解析是否成功
    if (ec.value())  // ec.value() == 0 表示成功，非 0 表示出错
    {
        // 打印错误码和错误消息
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();  // 返回非 0 表示程序异常退出
    }

    // ===== 组装 endpoint（IP + 端口）=====
    // 类似 C 语言做了 bind(addr) 中的地址准备工作
    boost::asio::ip::tcp::endpoint ep(ip_addr, port);

    return 0;
}
```

**`server_end_point()` 的区别**——服务器端需要监听"本机所有网络接口"：

```cpp
int server_end_point()
{
    unsigned short port = 9190;
    // C 语言：addr.sin_addr.s_addr = INADDR_ANY  ← "监听本机所有 IP"
    // ASIO：ip::address_v4::any()
    boost::asio::ip::address ip_addr = boost::asio::ip::address_v4::any();

    boost::asio::ip::tcp::endpoint ep(ip_addr, port);
    return 0;
}
```

`address_v4::any()` 对应 C 语言的 `INADDR_ANY`（值为 0.0.0.0），意思是"这台机器上所有的 IPv4 地址"。如果你的机器有多个网卡（比如 eth0: 192.168.1.10, lo: 127.0.0.1），`any()` 让服务器同时监听所有网卡。

### 4.3 io_context——为什么每个 socket 都需要它？

在继续看 `create_tcp_socket()` 之前，必须先搞懂 `io_context`。

**`io_context` 在 ASIO 里管两件事**：

1. **资源分配**——ASIO 的 socket、acceptor 等对象的创建和销毁都由 `io_context` 管理
2. **事件循环**——`io_context.run()` 是 ASIO 的大心脏，处理所有异步操作

你可以把 `io_context` 类比成**一个总闸开关**：

```
io_context
   │
   ├── 管 socket 的创建和销毁（资源管理）
   ├── 管事件循环（epoll_wait → 回调）
   ├── 管定时器（asio::steady_timer）
   └── 管跨线程任务投递（post / dispatch）
```

**关键规则**：`tcp::socket` 在构造时必须传入一个 `io_context`：

```cpp
boost::asio::io_context ioc;        // 先有 io_context
tcp::socket sock(ioc);              // socket 绑定到这个 io_context 上
```

为什么？因为 ASIO 需要在 socket 内部记录它属于哪个 io_context，这样当 socket 上有数据到达时，ASIO 才能把事件通知给正确的 io_context。

**类比**：`io_context` 像一个电话总机，每个 `socket` 是一条分机线。总机（`ioc.run()`）不停地监听所有分机，哪条线响了（数据到了），就接起来执行对应的回调。

### 4.4 create_tcp_socket()——socket() 对应 tcp::socket

```cpp
int create_tcp_socket()
{
    // C 语言：int fd = socket(AF_INET, SOCK_STREAM, 0);
    // ASIO 需要：先有 io_context，然后用它创建 socket
    boost::asio::io_context ioc;

    // 指定协议族：IPv4
    // C 语言的 AF_INET
    boost::asio::ip::tcp protocol = boost::asio::ip::tcp::v4();

    // 创建 socket 对象（此时还没打开底层 fd，也叫"未打开状态"）
    boost::asio::ip::tcp::socket sock(ioc);

    boost::system::error_code ec;

    // 打开 socket：相当于 C 语言的 socket(AF_INET, SOCK_STREAM, 0)
    // protocol 告诉内核用 IPv4 TCP
    sock.open(protocol, ec);

    // 检查是否成功
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }

    // 函数结束时 sock 离开作用域 → 析构函数自动 close(fd)
    // 不需要手动调用 close()！
    return 0;
}
```

**两种创建 socket 的方式**：

| 方式 | 代码 | 说明 |
|------|------|------|
| 两步式 | `tcp::socket sock(ioc);` + `sock.open(protocol, ec)` | 先创建对象，再打开底层 fd |
| 一步式 | `tcp::socket sock(ioc, protocol);` | 构造函数直接打开 fd |

两种等效。`endpoints.cpp` 用了两步式，让你看到每个步骤。

**对比 C 语言**：

```c
// C 语言：创建 + 打开一步完成
int fd = socket(AF_INET, SOCK_STREAM, 0);
if (fd == -1) {
    perror("socket");  // 查看 errno
    return -1;
}
// 记得在结束前 close(fd);
```

**ASIO 的进步**：
- 用 `error_code` 而不是全局的 `errno`（不会因为多线程互相覆盖）
- 析构函数自动 `close()`，不会忘记关 fd
- 类型安全——`tcp::socket` 不会跟 UDP 的 `udp::socket` 搞混

### 4.5 create_acceptor_socket()——socket + bind + listen 一步到位

这是 ASIO 最方便的地方之一：

```cpp
int create_acceptor_socket()
{
    boost::asio::io_context ioc;

    // ===== 一行 = 三件事 =====
    // 1. 创建 acceptor（内部创建了 socket fd）
    // 2. bind(9190) —— 监听 9190 端口
    // 3. listen() —— 开始监听
    //
    // 对照 C 语言：
    //   int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    //   bind(listen_fd, ...);
    //   listen(listen_fd, SOMAXCONN);
    boost::asio::ip::tcp::acceptor acceptor(
        ioc,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9190)
    );
    return 0;
}
```

**acceptor的构造函数做了 socket + bind + listen 三步**。acceptor 专门负责"等别人来连接"，所以它包含了 `bind()` 和 `listen()` 的逻辑。

对比 C 语言：

```c
// C 语言：三行代码
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(9190);
addr.sin_addr.s_addr = INADDR_ANY;
bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
listen(listen_fd, SOMAXCONN);
```

你看到了吗？ASIO 把 `bind()` 和 `listen()` 的参数直接放到了 `endpoint` 里，构造函数替你全部搞定。这就是 **ASIO 的抽象层次更高**——你不需要关心底层是 `bind()` 先还是 `listen()` 先，也不需要管 `sockaddr_in` 怎么填。

### 4.6 bind_acceptor_socket()——如果需要分开写

有时候你想把 socket、bind、listen 分开写以便处理错误：

```cpp
int bind_acceptor_socket()
{
    unsigned short port = 9190;
    // 创建 endpoint（IP: 0.0.0.0, 端口: 9190）
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port);

    boost::asio::io_context ioc;

    // 第一步：创建 acceptor（只打开 socket，不 bind）
    // acceptr 的 protocol 从 ep 获取
    boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());

    boost::system::error_code ec;

    // 第二步：手动 bind
    acceptor.bind(ep, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }

    // 注意：这里没有显式调用 listen()！
    // acceptor 在 accept() 或 async_accept() 被调用时自动 listen()
    return 0;
}
```

**acceptor 的常见构造方式总结**：

| 方式 | 代码 | 做了什么 |
|------|------|---------|
| 一步式 | `acceptor(ioc, tcp::endpoint(tcp::v4(), 9190))` | socket + bind + listen |
| 两步式变体 | `acceptor(ioc)` → `acceptor.bind(ep)` | 先创建，再 bind（accept 自动 listen） |
| 三步式完整 | `acceptor(ioc, ep.protocol())` → `bind()` → `listen()` | 每一步都可控 |

### 4.7 connect_to_end()——connect() 对应 socket.connect()

```cpp
int connect_to_end()
{
    std::string raw_ip_addr = "192.168.1.124";
    unsigned short port = 9190;

    // 这次用了 try-catch 方式（另一种错误处理）
    try
    {
        // 创建 endpoint（目标服务器地址 + 端口）
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);

        boost::asio::io_context ioc;
        // 创建 socket 并指定协议（一步式）
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol());

        // ===== 连接到服务器 =====
        // C 语言：connect(fd, (sockaddr*)&addr, sizeof(addr))
        // ASIO：sock.connect(ep)
        sock.connect(ep);

        return 0;
    }
    catch (boost::system::system_error &se)  // ← 捕获 ASIO 异常
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}
```

**C 风格 connect**：

```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(9190);
inet_aton("192.168.1.124", &server_addr.sin_addr);

if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("connect");
    close(fd);
    return -1;
}
```

**ASIO 的进步**：不需要自己管 `sockaddr_in`、`htons`、结构体类型转换。一切由 `endpoint` + `connect()` 封装好了。

### 4.8 dns_connect_to_end()——resolver 帮你查 DNS

这是传统 socket 编程里最烦人的部分之一——你需要用 `gethostbyname()` 或 `getaddrinfo()` 来解析域名。

```cpp
int dns_connect_to_end()
{
    std::string host = "llfc.club";   // ← 用域名，不是 IP
    unsigned short port = 9190;
    boost::asio::io_context ioc;

    // ===== tcp::resolver：域名解析器 =====
    // C 语言：getaddrinfo("llfc.club", "9190", &hints, &result)
    // ASIO：resolver.resolve("llfc.club", "9190")
    boost::asio::ip::tcp::resolver resolver(ioc);

    try
    {
        // resolve() 返回一个"端点范围"（一个域名可能对应多个 IP）
        // 返回的是迭代器范围，可以用范围 for 遍历
        auto endpoints = resolver.resolve(host, std::to_string(port));

        // 取出第一个 endpoint（通常就够了，因为操作系统会优先返回最优 IP）
        auto ep = *endpoints.begin();    // 方式一
        // auto ep = endpoints.front();  // 方式二，等效

        // 创建 socket
        boost::asio::ip::tcp::socket sock(ioc);

        // ===== boost::asio::connect（注意不是 sock.connect）=====
        // 这个 connect 接受一个"端点列表"，ASIO 会逐个尝试，直到成功
        boost::asio::connect(sock, endpoints);

        std::cout << "Connected to: " << ep.endpoint() << std::endl;
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}
```

**resolver 的工作流程**：

```
resolver.resolve("llfc.club", "9190")
    │
    │  内部调用 getaddrinfo()（跨平台封装）
    ▼
返回一个迭代器范围，包含：
    ├── 端点1: 93.184.216.34:9190 (IPv4)
    ├── 端点2: 2606:2800:220:1:248:1893:25c8:1946:9190 (IPv6)
    └── ...（如果有多个 IP）
```

**为什么 `boost::asio::connect(sock, endpoints)` 而不是 `sock.connect(ep)`？**

- `sock.connect(ep)` —— 连接单个指定的 endpoint
- `boost::asio::connect(sock, endpoints)` —— 传入端点列表，它会自动遍历，逐个尝试连接，哪个成功了就用哪个

这在域名有多个 IP 时特别有用（比如一个域名同时有 IPv4 和 IPv6 地址，或者做了 DNS 负载均衡）。

**C 语言做 DNS 解析**：

```c
struct addrinfo hints;
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;    // 支持 IPv4 和 IPv6
hints.ai_socktype = SOCK_STREAM;

struct addrinfo *result;
int ret = getaddrinfo("llfc.club", "9190", &hints, &result);
if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    return -1;
}

// 遍历链表，尝试连接
struct addrinfo *rp;
for (rp = result; rp != NULL; rp = rp->ai_next) {
    int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd == -1) continue;
    if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
        break;  // 连接成功
    }
    close(fd);
}
if (rp == NULL) {
    fprintf(stderr, "Could not connect\n");
    return -1;
}
freeaddrinfo(result);
```

几十行 C 代码的工作，ASIO 用两行搞定：

```cpp
auto endpoints = resolver.resolve(host, port_str);
boost::asio::connect(sock, endpoints);
```

### 4.9 accept_new_connection()——完整的 accept 流程

```cpp
int accept_new_connection()
{
    const int BACKLOG_SIZE = 30;   // 等待队列最大长度
    unsigned short port = 9190;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port);
    boost::asio::io_context ioc;

    try
    {
        // 第一步：创建 acceptor，只打开 socket
        boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());

        // 第二步：bind
        acceptor.bind(ep);

        // 第三步：listen（C 语言第二个参数是 backlog）
        acceptor.listen(BACKLOG_SIZE);

        // 第四步：（同步）接受连接
        // C 语言：int conn = accept(listen_fd, NULL, NULL)
        // ASIO：acceptor.accept(sock)
        // 「阻塞」直到有客户端连接上来
        boost::asio::ip::tcp::socket sock(ioc);
        acceptor.accept(sock);

        // 到这里，sock 已经连接到客户端了
        // 可以用 sock.read_some() / sock.write() 收发数据
        // 但本函数只是展示 accept 流程，所以直接返回了

        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}
```

**对照 C 语言**：

```c
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(9190);
addr.sin_addr.s_addr = INADDR_ANY;
bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));

listen(listen_fd, 30);

struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);
int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
```

**ASIO vs C 对照图**：

```
C socket 流程:                   ASIO 流程:

socket()                tcp::acceptor acceptr(ioc, ep.protocol())
    │                              │
bind()                  acceptor.bind(ep)
    │                              │
listen()                acceptor.listen(BACKLOG_SIZE)
    │                              │
accept()                acceptor.accept(sock)
    │                              │
[通信] sock = listen fd?  → 这里有个重要区别!
```

**重要区别**：C 语言的 `accept()` 返回的是一个新的 fd（`conn_fd`），而 `listen_fd` 继续等下一个连接。ASIO 也一样——`acceptor` 是监听者，`sock` 是通信用的，两者是不同的对象。

### 4.10 ASIO 的错误处理——两种方式

你在 `endpoints.cpp` 里看到了**两种错误处理方式**，必须搞懂：

| 方式 | 使用场景 | 代码模式 |
|------|---------|---------|
| **error_code** | 你期望这个操作可能失败，你想主动检查 | 传入 `error_code& ec` 参数，调用后检查 `ec.value()` |
| **system_error 异常** | 操作失败是"不正常的"，你想写在 `try-catch` 里 | 不传 ec 参数，失败时抛出 `boost::system::system_error` |

**方式一：error_code（主动检查）**

```cpp
boost::system::error_code ec;
sock.open(protocol, ec);       // 不抛异常，错误放到 ec
if (ec.value()) {              // 非0 = 有错
    std::cout << ec.message(); // 打印错误描述
}
```

**方式二：system_error 异常（try-catch）**

```cpp
try {
    sock.connect(ep);  // 不传 ec，失败时抛异常
} catch (boost::system::system_error &se) {
    std::cout << se.code().value() << ": " << se.what();
}
```

**哪个更好？**

- 同步代码里两种都可以。初学者建议用 **try-catch**，因为代码更干净（少写很多 ec 变量）。
- 异步回调里**只能用 error_code**——因为回调函数是在异步上下文中调用的，抛异常没法被外层 catch。
- 看 `endpoints.cpp` 的规律：有 `error_code` 参数时用检查方式，没有时用 try-catch。

### 4.11 把 endpoints.cpp 跑起来

编译这个文件（假设它包含 `main()` 调用这些函数）：

```bash
g++ -std=c++17 endpoints.cpp -lboost_system -lpthread -o endpoints
```

需要链接的库：

| 链接项 | 作用 |
|--------|------|
| `-lboost_system` | Boost 系统库——提供 `error_code`、`system_error` 等基础类型 |
| `-lpthread` | POSIX 线程库——ASIO 内部会用线程来执行回调 |

> **如果报错找不到头文件**：先安装 `sudo apt install libboost-dev libboost-asio-dev`

### 4.12 小结：C socket → ASIO 口诀

```
C 的 socket()  →  ASIO 的 tcp::socket sock(ioc)
C 的 bind()    →  ASIO 的 acceptor.bind(ep)
C 的 listen()  →  ASIO 的 acceptor.listen(backlog)
C 的 accept()  →  ASIO 的 acceptor.accept(sock)
C 的 connect() →  ASIO 的 sock.connect(ep)
C 的 gethostbyname → ASIO 的 resolver.resolve(host, port)
C 的 close()   →  ASIO 的 自动析构（RAII）
C 的 INADDR_ANY → ASIO 的 address_v4::any()
C 的 perror()  →  ASIO 的 ec.message() 或 se.what()
```

记住这张表，你再看到 `endpoints.cpp` 就不会觉得陌生了——它只是在用 C++ 的对象封装你熟悉的 C socket 操作。

---

## 5. 同步 I/O vs 异步 I/O

这是理解 ASIO 最重要的一步。我们做个类比。

### 5.1 去咖啡店点咖啡

| 模型 | 描述 | 对应代码 |
|------|------|---------|
| **同步阻塞** | 你站在柜台前等咖啡做好，期间啥也不干 | `recv(sock, buf, size, 0)` — 没数据就一直等 |
| **同步非阻塞** | 你每隔 5 秒去问"咖啡好了吗？"——没好就去旁边转转再回来问 | `fcntl(sock, F_SETFL, O_NONBLOCK)` + 循环 `recv()` 返回 `EAGAIN` |
| **异步非阻塞** | 你给店员留了电话"做好了打给我"，然后你该干嘛干嘛 | 这就是 **ASIO 的 `async_read`**——注册回调，io_context 跑起来，数据来了自动调用你的函数 |

表格里的"阻塞"指的是 **调用者在等待时能不能干别的事**。

### 5.2 代码对比——同步 vs 异步

**同步（传统 socket）：**

```cpp
// 1. 阻塞等待连接
int conn = accept(listen_fd, nullptr, nullptr);

// 2. 阻塞等待数据
char buf[1024];
int n = recv(conn, buf, sizeof(buf), 0);

// 3. 处理数据
process(buf, n);
// recv 没返回 → 线程/进程就卡在那了
```

**异步（ASIO）：**

```cpp
// 1. 异步接受连接（立即返回，不阻塞）
acceptor.async_accept(socket, [](error_code ec, tcp::socket s) {
    // 3. 连接来的时候，这个函数才会被调用
    async_read(s, buffer(buf), [](error_code ec, size_t n) {
        // 4. 数据来了，在这里处理
        process(buf, n);
    });
});
// 2. 这行立即执行，不会等连接
io.run();  // ← 事件循环，没有连接时内核帮忙等着
```

**关键区别**：同步代码按**顺序写**，程序"卡"在每一行直到它完成。异步代码按**回调写**，你告诉系统"数据到了叫我"，程序可以同时等很多事情。

---

## 6. 内核到底怎么收发数据的

这一段是**底层核心**，请慢慢看。

### 6.1 从应用程序到网卡的完整路径

当你的程序调用 `send(sock, "hello", 5, 0)` 时，这 5 个字节经历了什么？

```
你的程序（用户态）
    │
    │  send("hello")
    ▼
C 标准库（glibc） 
    │
    │  封装系统调用（软中断）  
    ▼
系统调用入口（内核态） ← 从这里开始进入内核
    │
    │  sys_sendto() / sys_write()
    ▼
Socket 层（内核）
    │
    │  1. 通过 fd 找到对应的 socket 结构体
    │  2. 检查 socket 状态（是否已连接、是否阻塞）
    │  3. 加锁（多个线程可能同时操作同一个 socket）
    ▼
TCP 协议栈（内核）
    │
    │  1. 将数据拷贝到 sk_buff（内核的网络缓冲区）
    │  2. 根据 TCP 状态机处理（是否需要三次握手？窗口够不够？）
    │  3. 如果数据量小，等待 Nagle 算法决定是否立即发
    │  4. 计算校验和
    │  5. 分片（如果数据超过 MSS）
    ▼
IP 层（内核）
    │
    │  1. 添加 IP 头（源 IP、目标 IP、TTL 等）
    │  2. 路由查找（决定从哪个网卡出去）
    │  3. 可能需要分片（如果超过 MTU）
    ▼
网络设备层（内核）
    │
    │  1. 添加链路层头（以太网帧头 — MAC 地址）
    │  2. 将 sk_buff 放入网卡驱动的发送队列
    ▼
网卡驱动 + 硬件
    │
    │  1. 驱动从队列取出 sk_buff
    │  2. 通过 DMA（直接内存访问）将数据拷贝到网卡内部缓冲区
    │  3. 网卡把数字信号调制成电信号/光信号发出去
    ▼
网线/光纤 → 路由器 → …… → 目标机器
```

**整个过程里，你的程序只做了 1 件事：调用了 `send()`。** 其他所有步骤都是内核 + 硬件完成的。

### 6.2 接收路径——更复杂

`recv()` 的路径更长，因为内核需要"等待数据到达"：

```
网卡收到电信号
    │
    │  DMA 将数据写入内存（不需要 CPU 参与！）
    ▼
网卡触发硬件中断（告诉 CPU "数据到了"）
    │
    │  中断处理程序（关中断，尽快处理）
    ▼
软中断（下半部，开中断）
    │
    │  1. 从网卡队列取出数据，构造 sk_buff
    │  2. 协议栈逐层向上：链路层 → IP 层 → TCP 层
    │  3. TCP 层：检查序列号、去重、重组、触发 ACK
    │  4. 如果 socket 有数据在等，唤醒等待的进程
    ▼
Socket 接收队列
    │
    │  sk_buff 被放入 socket 的接收缓冲区
    ▼
你的程序（用户态）
    │
    │  recv() 从内核缓冲区拷贝数据到用户缓冲区
    ▼
完成
```

### 6.3 缓冲区——内核里的"蓄水池"

**核心概念**：内核为每个 socket 维护了两个缓冲区——**发送缓冲区**和**接收缓冲区**。

```
                    ┌─────────────────────┐
    send() ─────────┤  发送缓冲区          │───→ 网卡
                    │  （内核内存）          │
                    └─────────────────────┘

                    ┌─────────────────────┐
    网卡 ──────────┤  接收缓冲区          │───→ recv()
                    │  （内核内存）          │
                    └─────────────────────┘
```

**发送过程**：`send()` 把数据从**用户内存**拷贝到**内核的发送缓冲区**就返回了（对于 TCP）。数据什么时候真正发出去、怎么发，是内核的事。

**接收过程**：网卡收到数据，内核把它放进**接收缓冲区**。你的 `recv()` 调用把数据从内核缓冲区拷贝到**用户内存**。

**为什么要有缓冲区？**
- 发送端：写得快，网卡发得慢 → 缓冲区帮你排队
- 接收端：网卡收得快，程序读得慢 → 缓冲区帮你暂存

### 6.4 阻塞的本质——进程状态切换

当你的程序调用 `recv()` 但接收缓冲区为空时，会发生：

```
recv(空缓冲区)
    │
    │  1. 系统调用进入内核
    ▼
内核发现缓冲区为空
    │
    │  2. 如果 socket 是阻塞的
    ▼
把当前进程标记为 "可中断睡眠" (TASK_INTERRUPTIBLE)
    │
    │  3. 将进程放入 socket 的等待队列
    ▼
调用调度器 (schedule())
    │
    │  4. CPU 切换到其他进程去干活了
    ▼
你的进程"睡着了"，不占 CPU
                                        ← 网卡收到数据，软中断处理
                                        ← 内核将数据放入接收缓冲区
                                        ← 内核检查等待队列，找到你的进程
                                        ← 把进程状态改回 "就绪" (TASK_RUNNING)
                                        ← 调度器下次选中你的进程
                                        │
你的进程恢复执行                        │
    │                                   │
    │  5. 从内核缓冲区拷贝数据到用户内存  │
    ▼                                    ▼
recv() 返回，程序继续
```

这就是**阻塞**的本质：**进程主动让出 CPU，进入睡眠状态，内核在数据到达时唤醒它。**

**阻塞不浪费 CPU**——因为调度器会把 CPU 给别的进程。阻塞浪费的是**线程资源**：每个连接一个线程，一千个连接就需要一千个线程的栈空间（每个线程默认 8MB，一千个就是 8GB）。

---

## 7. 阻塞与非阻塞——内核视角

我们再看一下**非阻塞**的情况下，内核做了什么：

```
recv(空缓冲区，但 socket 设为非阻塞)
    │
    │  系统调用进入内核
    ▼
内核发现缓冲区为空
    │
    │  检查 socket 标志：哦，是非阻塞的
    ▼
不睡眠！立即返回 -1，并设置 errno = EAGAIN
    │
    │  意思是："现在没数据，但你也不用等，去干别的事吧"
    ▼
recv() 立即返回，你的程序继续执行
```

**非阻塞不睡眠，但也不告诉你数据什么时候到**。所以程序需要**不断轮询**——每隔几毫秒问一次"数据到了吗？"。轮询浪费 CPU。

**所以 select/poll/epoll 登场了**——它们让你"一次等很多个 fd"，哪个有数据就告诉哪个，既不用多线程，也不用轮询：

```
epoll_wait(fds, 100)  ← 等最多 100ms
    │
    │  如果没有 fd 有数据，进程睡眠（不浪费 CPU）
    │  如果有 fd 有数据，立即返回哪些 fd 就绪了
    ▼
返回：fd 3 可读，fd 5 可写
```

**epoll 的底层**：内核在数据到达（软中断处理）时，除了把数据放进接收缓冲区，还会检查这个 socket 是否被 epoll 监控。如果是，就把这个 socket 加入 epoll 的就绪队列。然后唤醒正在 `epoll_wait` 的进程。

```
网卡中断 → 软中断
    │
    │  1. 数据→sk_buff→接收缓冲区
    │  2. 检查 socket 是否被 epoll 关注
    │  3. 是 → 把 socket 加入 epoll 的就绪链表
    │  4. 如果有进程在 epoll_wait 上睡眠 → 唤醒它
    ▼
epoll_wait 返回，你的程序处理就绪的 fd
```

---

## 8. I/O 模型演进：从多进程到 Proactor

我们串一下整个演进路线，你就知道 ASIO 站在哪个位置：

```
                  I/O 模型进化树

    最原始             阻塞 I/O（一个连接一个线程）
                           │
    多路复用              select/poll（单线程等所有 fd）
                           │
    事件驱动              epoll（Linux 高性能版 select）
                           │
    异步框架              ASIO（封装 epoll/kqueue/IOCP）
                           │
    现代异步              C++20 coroutines + ASIO
```

### 8.1 各阶段的代价

| 阶段 | 写法 | 代价 |
|------|------|------|
| 多进程/多线程 | `accept` + `fork`/`thread` | 每连接 8MB 栈 + 切换开销 |
| select/poll | 单线程 + `select()` 等所有 fd | 每次调用传递全部 fd 给内核（O(N)） |
| epoll | `epoll_create` + `epoll_ctl` + `epoll_wait` | 只传变化的 fd（O(1)），但代码复杂 |
| ASIO | `async_accept` + 回调 | 跨平台，代码简洁，但回调嵌套深 |

### 8.2 Reactor vs Proactor

这两个词经常被提到，简单区分：

| 模式 | 谁做实际读写 | 举例 |
|------|------------|------|
| **Reactor**（反应器） | 你（应用程序）自己调用 `read`/`write` | epoll、select、libevent |
| **Proactor**（前摄器） | ASIO 内核帮你读写，读好了告诉你 | ASIO（Windows 上是这样，Linux 上是模拟的） |

**Reactor**："内核说数据到了，你自己去读吧。"

**Proactor**："内核直接把数据读到你给的 buffer 里了，现在给你回调通知。"

ASIO 在 Windows 上是**真 Proactor**（利用 IOCP，内核帮你做所有读写），在 Linux 上是用 epoll **模拟**的 Proactor（ASIO 内部帮你做读，你只收到"读完了"的回调）。

---

## 9. ASIO 的 Proactor 模型

ASIO 的核心概念只有 4 个，必须理解：

### 9.1 io_context——事件循环引擎

`io_context` 是 ASIO 的心脏。你可以把它理解成一个**任务队列**：

```
io_context
    │
    ├── 待处理的任务队列（"数据到了请调用这个回调"）
    ├── 当前正在执行的任务
    ├── 底层平台抽象（Linux 上用 epoll，Windows 上用 IOCP）
    │
    └── run() 方法：启动事件循环
```

**`io_context::run()` 做了什么？**

```
io.run()
    │
    └── 无限循环（直到没有待处理的任务）：
        │
        ├── 1. 调用 epoll_wait（或等效的系统调用）
        │      → 没有事件时，线程阻塞在这里（不占 CPU）
        │
        ├── 2. 有事件了，从 epoll 拿到就绪的 fd 列表
        │
        └── 3. 对每个就绪的事件，调用对应的回调函数
               → 回调函数执行你的业务逻辑
               → 回调里可能又注册了新的异步操作
               → 回到步骤 1
```

**注意**：`io.run()` **阻塞当前线程**，直到所有异步操作都完成且没有新的操作注册。它其实等价于你手写的：

```cpp
while (有未完成的任务) {
    epoll_wait(...);
    for (每个就绪的 fd) {
        执行回调();
    }
}
```

### 9.2 socket——网络套接字的封装

ASIO 的 `tcp::socket` 是对底层 socket fd 的 C++ 封装，但比你手写的 `int fd` 多了：

- RAII 管理（析构函数自动 `close()`）
- 异步接口（`async_read_some`、`async_write_some`）
- 错误处理（通过 `error_code`，而不是全局的 `errno`）

### 9.3 acceptor——接受连接

`tcp::acceptor` 对应传统 socket 的 `bind()` + `listen()` + `accept()` 三部曲。它的 `async_accept()` 是对 `accept4()` 系统调用的异步封装。

### 9.4 buffer——数据缓冲区

`asio::buffer()` 是你给 ASIO 的"把数据放这里"的地址。它只是一个包装——ASIO 不会拷贝你的数据，只是记下指针和长度。

**重要**：你传给 `async_read` 的 buffer 在回调执行之前**必须保持有效**（不能是局部变量）。

---

## 10. 第一个 ASIO 例子——简单的 TCP 服务器

### 10.1 完整的 echo 服务器

```cpp
#include <boost/asio.hpp>
#include <iostream>
#include <memory>  // std::enable_shared_from_this

using boost::asio::ip::tcp;

// Session 类：处理一个客户端的整个生命周期
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket)
        : socket_(std::move(socket))  // 把 acceptor 传下来的 socket 拿过来
    {
    }

    void start()
    {
        do_read();  // 开始第一次异步读
    }

private:
    void do_read()
    {
        // 1. 注册异步读操作
        //    async_read_some 是"读一些数据，有多少读多少"（不保证读完你给的 buffer 大小）
        //    如果要保证读完指定字节数，用 async_read
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),  // 读到 data_ 里
            [self = shared_from_this()]               // 回调：捕获智能指针防止 session 被销毁
            (boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    // 2. 读到了数据，原样写回去（echo）
                    self->do_write(length);
                }
                // 如果有错误（比如客户端断开），session 会随 shared_ptr 自动析构
                // 析构函数自动关闭 socket
            });
    }

    void do_write(std::size_t length)
    {
        // 异步写：把 data_ 里的 length 字节写回去
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(data_, length),
            [self = shared_from_this()]
            (boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 写完了，继续读（下一个请求）
                    self->do_read();
                }
            });
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(boost::asio::io_context& io, short port)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port))  // bind + listen
        , socket_(io)                                     // 创建一个空 socket（稍后被 acceptor 填充）
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(socket_,  // 接受新连接
            [this](boost::system::error_code ec) {
                if (!ec) {
                    // 创建一个 Session 来处理这个客户端
                    std::make_shared<Session>(std::move(socket_))->start();
                }
                // 继续等待下一个连接（形成循环）
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
    tcp::socket    socket_;  // 用于 accept 的"模板"socket
};

int main()
{
    try {
        boost::asio::io_context io;  // 1. 创建事件循环引擎
        Server server(io, 8888);     // 2. 创建服务器（绑定 8888 端口）
        std::cout << "Server running on port 8888" << std::endl;
        io.run();                    // 3. 启动事件循环（阻塞在这里）
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
```

### 10.2 这段代码的"执行流"拆解

```
main()
  │
  ├── io_context io;       ← 创建事件引擎（此时还没启动）
  ├── Server server(io, 8888);
  │     │
  │     └── acceptor_.async_accept(...)
  │           │
  │           └── 注册了一个"有新连接就叫我"的任务到 io_context
  │
  ├── io.run();
  │     │
  │     └── 循环开始：
  │           │
  │           ├── epoll_wait() — 等连接或数据
  │           │
  │           ├── 有新连接 → 回调 do_accept()
  │           │     │
  │           │     ├── 创建 Session → start() → do_read()
  │           │     │     │
  │           │     │     └── async_read_some() 注册读任务
  │           │     │
  │           │     └── do_accept() 再次注册等待下一个连接
  │           │
  │           ├── 有数据 → 回调 Session::do_read()
  │           │     │
  │           │     └── do_write() → async_write() 注册写任务
  │           │
  │           ├── 写完成 → 回调 Session::do_write()
  │           │     │
  │           │     └── do_read() 再次注册读任务
  │           │
  │           └── ……无限循环……
```

**关键点**：`do_accept()` → `do_read()` → `do_write()` 互相调用，但不是**递归**——每次调用只是注册一个任务到 io_context 然后立即返回，真正的执行由 `io.run()` 里的循环驱动。

### 10.3 编译运行

```bash
g++ -std=c++20 server.cpp -lboost_system -lpthread -o server
./server
# 另一个终端：
nc localhost 8888
# 输入任意文字，服务器原样返回
```

链接 `-lboost_system`（Boost 系统库）和 `-lpthread`（ASIO 内部用线程）。

---

## 11. 第一个 ASIO 例子——客户端

```cpp
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main()
{
    try {
        boost::asio::io_context io;

        // 1. 创建 socket
        tcp::socket socket(io);

        // 2. 连接服务器（同步方式，简单演示）
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "8888");
        boost::asio::connect(socket, endpoints);

        std::cout << "Connected to server" << std::endl;

        // 3. 发送数据
        std::string msg = "Hello from ASIO client!";
        boost::asio::write(socket, boost::asio::buffer(msg));

        // 4. 接收回复
        char reply[1024];
        size_t len = socket.read_some(boost::asio::buffer(reply));
        std::cout << "Server replied: " << std::string(reply, len) << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
```

这个客户端用的是**同步**接口（`write` 和 `read_some`），跟之前学的传统 socket 一样——每步阻塞。但好处是 ASIO 帮你处理了跨平台差异（不用管 Windows 和 Linux 的 socket API 区别）。

---

## 12. ASIO 的底层——不用 epoll 也行？

### 12.1 平台适配层

ASIO 定义了一个**"平台后端"**的抽象层。同一套 API 在不同系统上自动选择不同的后端：

| 平台 | ASIO 后端 | 系统调用 |
|------|-----------|---------|
| Linux | **epoll** | `epoll_create`、`epoll_ctl`、`epoll_wait` |
| macOS / iOS / FreeBSD | **kqueue** | `kqueue`、`kevent` |
| Windows | **IOCP**（I/O Completion Port） | `CreateIoCompletionPort`、`GetQueuedCompletionStatus` |
| Windows (备选) | **select** | 老旧的 `select`（性能差） |

**ASIO 在编译时通过宏判断平台**，选择对应的实现。你在代码里永远只看到 `io_context`、`async_read`、`async_write`。

### 12.2 Linux 上 ASIO 的 epoll 封装

ASIO 不会直接把 epoll 暴露给你，但理解它内部怎么用 epoll 有助于你写更好的代码：

```cpp
// ASIO 内部（伪代码，示意epoll的使用方式）

class io_context::impl {
    int epoll_fd_;  // epoll 实例的文件描述符

    void run() {
        while (!stopped_) {
            // epoll_wait 最多等 1 毫秒（如果有定时任务则按最近定时器算）
            int n = epoll_wait(epoll_fd_, events_, max_events, 1);

            for (int i = 0; i < n; i++) {
                // 每个 events[i] 对应一个 fd
                // events[i].data.ptr 指向一个 "操作描述符"（operation）
                auto op = static_cast<operation*>(events[i].data.ptr);
                op->complete();  // 执行回调
            }

            // 处理定时器
            check_timers();

            // 处理 post 的任务（其他线程投递过来的）
            execute_posted_tasks();
        }
    }

    void async_read(socket& s, buffer b, callback c) {
        // 创建一个 "读操作" 对象，包含 socket、buffer、回调
        auto op = new read_operation(s, b, c);

        // 把这个 socket 加入 epoll 关注（EPOLLIN — 可读事件）
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, s.native_handle(),
                  {EPOLLIN, {.ptr = op}});
    }
};
```

**关键洞察**：ASIO 的每个异步操作（`async_read`、`async_accept` 等）都会在堆上创建一个 operation 对象，通过 `epoll_ctl` 把它的指针"挂"到 epoll 上。当 epoll 说"这个 fd 有事件了"，ASIO 直接从 `epoll_event.data.ptr` 拿到对应的 operation 对象，调用它的 complete 方法（也就是你写的那个回调 lambda）。

**所以每个异步操作都会有一次堆分配**——这是 ASIO 的性能成本之一。高性能场景下可以通过自定义 `executor` 和内存池优化。

### 12.3 真正发数据时——DMA 的故事

在数据从用户态到内核、从内核到网卡的传输中，有一个非常重要的硬件机制叫 **DMA（Direct Memory Access，直接内存访问）**。

没有 DMA 的时代：

```
CPU 从内核缓冲区读 1KB → 写到网卡寄存器 → CPU 被占住，不能干别的
CPU 从内核缓冲区再读 1KB → 再写到网卡寄存器 → CPU 又被占住
```

这样发送 1MB 数据，CPU 全程被占用做"搬砖"工作。

有了 DMA 后：

```
CPU 把"数据在哪、长度多少、写到哪"告诉 DMA 控制器 → CPU 去干别的事了
DMA 控制器自己把数据从内存搬到网卡 → 搬完了发一个中断通知 CPU
```

**DMA 控制器是一个独立的硬件**，它可以在 CPU 不参与的情况下，在内存和外设之间搬运数据。这就是为什么你的程序发 1GB 数据但 CPU 使用率不高——搬砖的工作交给 DMA 了。

```
你的程序 → send() → 内核缓冲区 ──→ DMA ──→ 网卡硬件 ──→ 网线
                              ↑ CPU 只负责第一次拷贝（用户→内核）
                              ↑ 内核→网卡由 DMA 完成
```

---

## 13. 总结

### 13.1 本文核心概念速查

| 概念 | 一句话 |
|------|--------|
| **Boost** | C++ 的第三方库大礼包，很多功能后来进了 C++ 标准 |
| **.hpp** | 就是 C++ 头文件，跟 `.h` 没本质区别，只是告诉别人"这是纯 C++" |
| **ASIO** | 跨平台的 C++ 异步网络库，Linux 用 epoll，Windows 用 IOCP |
| **io_context** | ASIO 的事件循环引擎，`run()` 启动无限循环 |
| **同步** | 你等数据，数据不到你不走（阻塞） |
| **异步** | 你告诉内核"数据到了叫我"，你继续干别的事（回调） |
| **内核缓冲区** | 每个 socket 在内核里有一个接收缓冲和一个发送缓冲 |
| **阻塞的本质** | 进程主动睡眠，让出 CPU，内核在数据到达时唤醒它 |
| **DMA** | 硬件级别的内存→外设数据搬运工，不需要 CPU 参与 |
| **Proactor** | ASIO 的设计模式——内核帮你读完数据再通知你 |

### 13.2 学习路线

如果你读完了这篇文章，接下来的学习顺序建议：

1. **编译运行上面的 echo server/client 例子**，亲手感受 ASIO 的程序结构
2. 把同步客户端改成异步（用 `async_write` + `async_read`）
3. 学习 `async_read` vs `async_read_some` 的区别（"读完指定字节" vs "读多少算多少"）
4. 学习 ASIO 的 `strand`（解决多线程下的回调并发问题）
5. 了解 C++20 coroutines + ASIO（用 `co_await` 写异步代码，告别回调嵌套）
6. 阅读 ASIO 官方文档或 Chris Kohlhoff 的论文（ASIO 原作者）

---

> **下一步**：在 `net_program/code/` 下写一个实际的 ASIO 程序试试？

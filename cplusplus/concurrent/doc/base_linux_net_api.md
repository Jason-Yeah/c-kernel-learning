# Linux 网络编程基础 API：从 C17 到 C++20

本文面向第一次接触 Linux 网络编程的读者，按游双《Linux 高性能服务器编程》第二篇第 5 章 **5.1～5.12** 编排。配套代码在 [base_linux_net_api](../code/base_linux_net_api/readme.md)。

**资料依据**：编写过程中发现并读取了本目录的 [书籍 PDF](<Linux高性能服务器编程 (游双) (Z-Library).pdf>)，已核对第 5 章正文。下面的“书中对照”以该文件阅读器从 1 开始的 PDF 页序号定位（第 5 章从 PDF 第 168 页开始；不是纸质版页码），用自己的话归纳，不复制原书长段落或整段程序。传统知识与现代 Linux 差异分别说明；接口行为另按链接的 Linux man-pages 校验。配套程序为本教程重新编写。

你说的“C11-C20”在这里分成两条线：**C11/C17 是 C 语言标准；C++11/14/17/20 是 C++ 标准**。不存在对应的正式“C20”标准；C++20 也不等于 C20。本文用 C17 展示底层调用，用 C++20 展示资源管理，并解释哪些能力从 C++11 就有。

## 阅读路线与目录

先读第 0 节建立概念，再按章节顺序阅读。第一次可以先跳过带外数据实验，优先运行 TCP，再运行 UDP。

- [0. 必备语言知识与 OS 模型](#0-必备语言知识与-os-模型)
- [5.1 socket 地址 API](#51-socket-地址-api)
- [5.2 创建 socket](#52-创建-socket)
- [5.3 命名 socket](#53-命名-socket)
- [5.4 监听 socket](#54-监听-socket)
- [5.5 接受连接](#55-接受连接)
- [5.6 发起连接](#56-发起连接)
- [5.7 关闭连接](#57-关闭连接)
- [5.8 数据读写](#58-数据读写)
- [5.9 带外标记](#59-带外标记)
- [5.10 地址信息函数](#510-地址信息函数)
- [5.11 socket 选项](#511-socket-选项)
- [5.12 网络信息 API](#512-网络信息-api)
- [6. 从基础 API 到现代高性能服务器](#6-从基础-api-到现代高性能服务器)
- [7. 调试与复习](#7-调试与复习)

## 原书与本文如何配合阅读

这份 PDF 的第 5 章位于阅读器第 **168～222 页**。以下页码都是 PDF 页序号。可先读本文建立概念，再回到对应页观察原书实验；原书代码包含排版或接口细节问题，不建议直接复制编译。

| 原书小节 | PDF 页码 | 原书主线 | 本文补充与配套实验 |
|---|---|---|---|
| 5.1 | 168～176 | 字节序、地址结构、文本转换 | C/C++ 类型与初始化、IPv6、返回值区分 |
| 5.2 | 177～178 | socket 参数与描述符 | RAII、CLOEXEC 与 exec 的关系 |
| 5.3 | 179～180 | bind 与绑定错误 | 回环地址、通配地址、端口 0 |
| 5.4 | 181～184 | backlog 队列实验 | 区分观察结果与 API 保证 |
| 5.5 | 185～188 | 延迟 accept、客户端提前退出 | listener/peer 所有权、accept4 |
| 5.6 | 189 | connect 与失败原因 | 非阻塞完成检查、UDP connect |
| 5.7 | 190～191 | close 与 shutdown | 半关闭时序、引用与析构 |
| 5.8 | 192～200 | TCP、UDP、带外收发、msghdr | 完整读写、分帧、零长度报文、截断实验 |
| 5.9 | 201 | 紧急事件与 sockatmark | inline 带外字节实验 |
| 5.10 | 202 | 本端与对端地址查询 | 查询动态端口与连接日志 |
| 5.11 | 203～213 | 地址复用、缓冲区、低水位、linger | 当前 Linux 限制、历史配置说明 |
| 5.12 | 214～222 | 旧查询接口、daytime、新查询接口 | 双栈解析、资源释放、离线可运行实验 |

**原书实验与配套代码的关系**：配套程序复现相关 API 的学习目标，并非逐份移植代码清单。尤其 backlog 堆积、断网后的延迟 accept 和缓冲区抓包实验，在本文中解释其原理，没有声称自动测试复现了这些网络条件。本文的长度前缀协议、畸形消息验证和 RAII 是额外补充。

## 0. 必备语言知识与 OS 模型

### 0.1 先理解一个请求如何走过去

进程是运行中的程序。服务器进程等请求，客户端进程主动请求；它们可以在同一台机器上。IP 地址帮助定位网络接口，端口帮助操作系统把传输层数据交给对应的 socket。`127.0.0.1` 是 IPv4 回环地址，发给它的数据留在本机协议栈里；`::1` 是 IPv6 回环地址。

socket（套接字）是操作系统提供的通信端点抽象。TCP 提供可靠、有序的字节流；UDP 发送独立数据报，不保证送达、顺序或去重。可靠也不意味着永远成功：断网后 TCP 最终仍可能超时失败。

```text
客户端用户内存                         服务端用户内存
    send()                                 recv()
      ↓                                      ↑
客户端内核发送缓冲区 → TCP/IP → 服务端内核接收缓冲区
                          ↑
                 网卡/网络，或本机回环
```

系统调用跨越用户态与内核态的权限边界，让内核检查参数、管理资源和访问协议栈。常规 socket I/O 涉及用户缓冲区与内核缓冲区之间的数据复制。调用一次 `send` 不对应发送一个 IP 包；内核可以合并、分段、重传。

**阻塞**是条件不满足时调用暂不返回，线程通常进入等待状态，让 CPU 执行别的任务。**非阻塞**是暂时不能完成时立即返回错误码 `EAGAIN` 或 `EWOULDBLOCK`。非阻塞不等于操作自动在后台完成，也不等于多线程。

### 0.2 fd、内核对象与引用

`socket()` 返回一个非负整数，称为文件描述符 fd。它是当前进程描述符表的索引，不是 IP 地址，不是内存指针，也不是全系统唯一的连接编号。`0` 也可能是有效 socket fd，因此必须用 `fd >= 0` 判断有效，失败值是 `-1`。

```text
进程 fd 表 → 打开文件描述（内核对象）→ socket 状态、协议、队列
   3 ───────────────┐
   4（dup 得到）─────┴─→ 同一个通信端点
```

`dup` 或 `fork` 可产生指向同一底层对象的引用。关闭一个 fd 不必然释放最后一个引用。fd 关闭后编号能立即复用，因此不能拿失效编号继续读写或重复关闭。C++ RAII 封装解决“谁拥有关闭责任”的问题；它不会替你设计应用协议。

### 0.3 读懂 C17 函数调用

```c
struct sockaddr_in addr = {0};
addr.sin_family = AF_INET;
addr.sin_port = htons(9000);
int rc = bind(fd, (struct sockaddr *)&addr, sizeof addr);
```

- `struct` 把几个字段放进一个类型；`.` 访问结构体对象字段，`p->field` 访问指针所指对象的字段。
- `= {0}` 初始化成员，避免把不确定的栈内存当成地址参数。
- `&addr` 取对象地址。函数借这个指针读取内容；`*p` 则表示访问指针指向的对象。
- `sizeof addr` 是该对象占用的字节数，不是它保存的字符串长度。
- `(struct sockaddr *)` 是 C 指针转换；这里为了适配通用 socket API，并没有创建新对象或转换 IP 数值。
- `const void*` 可以指向不同类型的数据，但调用者仍要正确提供长度，系统无法从指针推断缓冲区大小。
- `size_t` 表示非负大小；`ssize_t` 是有符号的字节计数类型，能表示 `-1`。先检查负值，再转换为 `size_t`。
- `socklen_t` 是 socket 地址/选项长度类型，不能想当然用 `int*` 代替 `socklen_t*`。
- `NULL` 是 C 中的空指针常量；C++11 起通常写 `nullptr`。

`errno` 是与线程关联的错误状态。大多数这里的系统调用以 `-1` 报错，随后才读取 `errno`；成功后不要把旧 `errno` 当成这次失败。`perror("bind")` 输出当前错误的文字解释。需要跨后续调用保存错误时先 `int saved = errno`。

`getaddrinfo/getnameinfo` 是例外：返回自己的错误码，用 `gai_strerror(rc)`；若为 `EAI_SYSTEM`，再检查系统错误 `errno`。

C17 中常用 `goto cleanup` 汇合错误路径，集中 `close`，这不是网络协议的一部分。这里禁止用 `assert(socket(...) >= 0)` 执行必需操作：关闭断言的构建可能连调用本身都删除。

### 0.4 读懂现代 C++ 封装

```cpp
sockaddr_in addr{};
check(::bind(fd.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)), "bind");
```

`{}` 是初始化；`::bind` 明确调用全局的 POSIX 函数，避免与 `std::bind` 混淆。`reinterpret_cast` 显式表达底层指针适配，仅在 API 要求的位置使用，不会检查指向的对象是否真的足够大。

`unique_fd` 是本例自定义类，不是标准库类型。构造后独占一个 fd，析构调用 `close`，禁止复制以避免两次关闭，允许移动来转移所有权。局部对象离开作用域（包括抛出异常）时自动析构，这叫 RAII：资源获取即初始化。

| 标准 | 本主题中有用的能力 | 不要误解成什么 |
|---|---|---|
| C17 | 结构体、指针、明确的错误与清理路径 | ISO C 并不定义 Linux socket API |
| C++11 | RAII、删除复制函数、移动语义、`unique_ptr`、`system_error` | `std::move` 本身不复制/搬运 socket |
| C++14 | `std::exchange` 简化转移所有权 | 不是网络系统调用 |
| C++17 | `string_view` 表示非拥有字符串视图、`std::byte` 表示字节 | 视图不会延长数据寿命 |
| C++20 | `span` 表示连续缓冲区、协程语言机制 | 标准库尚无通用 socket API，也无内置网络事件循环 |

`std::span<const char>` 只保存一段内存的地址和长度，不负责分配/释放；`subspan(n)` 跳过已发送部分。异步发送时原始内存必须一直存活到完成。`std::vector<char>` 才拥有动态缓冲区。

`throw std::system_error(...)` 把系统错误传播给上层；示例在 `main` 的 `catch` 中打印错误并退出。生产系统通常在单连接边界处理错误，不能因为一个坏请求退出整个服务器。

## 5.1 socket 地址 API

**书中对照**（归纳）：PDF 第 168～176 页：书中先用 union 观察整数的字节布局，再介绍四个字节序转换函数、通用/专用地址结构，最后比较旧 IPv4 转换与 inet_pton/inet_ntop。学习重点是“内存里的表示”和“线上协议表示”不同。现代 C++ 不宜照抄读取 union 非活动成员的探测方式；C++20 可用 `<bit>` 中的 `std::endian::native` 判断，但协议编码仍须明确转换。主机序并不必然是小端序。

**本节传统 API 学习重点**：先理解字节序，再区分通用地址与专用地址，最后进行文本 IP 与二进制地址转换。

### 5.1.1 主机字节序和网络字节序

内存按字节寻址，十六进制 `0x1234` 占两字节。大端序把 `12` 放在较低地址，小端序把 `34` 放在较低地址。网络协议中的这类多字节整数通常按大端序传输，不能依赖本机 CPU 的排列方式。

| 函数 | 含义 | 常见对象 |
|---|---|---|
| `htons` / `ntohs` | 主机与网络之间的 16 位转换 | 端口 |
| `htonl` / `ntohl` | 主机与网络之间的 32 位转换 | IPv4 整数、应用协议长度 |

`h/n/s/l` 分别帮助记忆 host/network/short/long；这里的 long 指 32 位协议转换，不等于所有平台的 C `long` 大小。大端主机上这些转换仍要写，以便代码可移植。

原书 PDF 第 170 页按旧式类型写出了转换函数原型。当前接口用 `uint32_t`/`uint16_t` 明确位宽；在常见 64 位 Linux 上 `unsigned long` 是 64 位，不能把函数名的 `l` 理解为任意机器的 long。实际开发包含系统头文件，不自行照书重写函数声明。参见 [byteorder(3)](https://man7.org/linux/man-pages/man3/byteorder.3.html)。

例如长度为 5 的消息使用四字节头 `00 00 00 05`，接收方读取这四字节后 `ntohl` 得到主机整数 5。字符串 `"9000"` 是四个字符，不能直接当端口整数；先解析数值再 `htons`。`inet_pton` 生成的地址已经是网络表示，不要再次 `htonl`。

### 5.1.2 通用 socket 地址

`struct sockaddr` 提供通用参数形式：地址族告诉内核后续内容采用什么布局。不要用它当成能装所有协议地址的大盒子；接收未知地址时用 `sockaddr_storage`，它提供足够的空间及对齐。

```cpp
sockaddr_storage peer{};
socklen_t length = sizeof(peer);  // 输入：容量
int connection = ::accept(listener,
    reinterpret_cast<sockaddr*>(&peer), &length);
// 成功后 length 是实际地址长度；检查 peer.ss_family 再解释地址。
```

长度是“值—结果参数”：调用前写容量，调用后函数写实际长度。每次重新接收地址时都要重新设置容量。

### 5.1.3 专用 socket 地址

| 地址族 | 类型 | 主要字段 |
|---|---|---|
| `AF_INET` | `sockaddr_in` | `sin_family`、`sin_port`、`sin_addr` |
| `AF_INET6` | `sockaddr_in6` | `sin6_family`、`sin6_port`、`sin6_addr`、`sin6_scope_id` |
| `AF_UNIX` | `sockaddr_un` | `sun_family`、`sun_path` |

IPv6 链路本地地址可能需要作用域标识来区分接口。UNIX 域 socket 用于同一台机器的进程间通信，可以用路径名；Linux 还有抽象命名空间，它不是普通磁盘文件。不要对所有地址族都传 `sizeof(sockaddr)`。

### 5.1.4 IP 地址转换函数

旧接口 `inet_addr/inet_aton/inet_ntoa` 主要面向 IPv4；`inet_ntoa` 返回的存储可能被后续调用覆盖。学习时认识它们，新代码通常使用 `inet_pton/inet_ntop`。

```cpp
in_addr binary{};
int rc = ::inet_pton(AF_INET, "127.0.0.1", &binary);
// rc == 1 成功；0 表示文本格式非法；-1 表示错误（例如不支持的地址族）。
char text[INET_ADDRSTRLEN]{};
const char* result = ::inet_ntop(AF_INET, &binary, text, sizeof(text));
// result == nullptr 表示失败；IPv6 使用 INET6_ADDRSTRLEN。
```

书中将 inet_pton 的失败情况概括得较简略；务必区分非法输入返回 0 与系统错误返回 -1，只有后者按 errno 解释。

原书代码清单 5-2 用文本字符串直接调用 `inet_ntoa` 来说明结果覆盖，但实际参数必须是 `struct in_addr`；需要先完成文本到二进制转换。它要说明的存储覆盖问题值得学习，示意代码本身不能原样照搬。

`p` 是 presentation（文本表示），`n` 是 network。它们不查询 DNS，不能把 `example.com` 当 IP 交给 `inet_pton`。现代 C++ 仍调用相同函数，改进在于缓冲区管理、检查返回值和支持 IPv6。参见 [inet_pton(3)](https://man7.org/linux/man-pages/man3/inet_pton.3.html)。

## 5.2 创建 socket

**书中对照**（归纳）：PDF 第 177～178 页：书中把 socket 放到 UNIX 文件描述符模型里解释，再逐项讲 domain/type/protocol。阅读时要修正两个细节：类型与标志通过按位或 `|` 组合；CLOEXEC 在成功 exec 时生效，不是 fork 时。相关 type 标志在 Linux 2.6.27 引入，不能照抄该 PDF 的旧版本号叙述。

```c
int socket(int domain, int type, int protocol);
```

**传统学习重点**：地址族、传输语义、协议三个参数共同定义通信端点。

```cpp
int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
```

这里 `AF_INET` 表示 IPv4，`SOCK_STREAM` 表示流式语义，协议 0 表示让系统选择适配协议，此组合为 TCP。IPv4/IPv6 的 `SOCK_DGRAM` 常用 UDP；不能脱离地址族断言所有 stream 都是 TCP。

`|` 是按位或，用来组合标志。`SOCK_CLOEXEC` 让进程成功执行 `exec` 换程序时关闭该 fd，避免无意泄漏给新程序；它不意味着 `fork` 时关闭。`SOCK_NONBLOCK` 可以在创建时设置非阻塞模式，减少创建后再修改标志的竞态。

**OS 原理**：此时内核分配 socket 相关对象，但 TCP 尚未完成连接，服务端也尚未监听；创建成功不代表对端存在。

**现代写法**：立刻将成功 fd 放进 `unique_fd`。失败时记录原因；`EMFILE` 是进程 fd 资源不足，`ENFILE` 是系统级文件资源不足，盲目重试不会解决资源泄漏。接口见 [socket(2)](https://man7.org/linux/man-pages/man2/socket.2.html)。

## 5.3 命名 socket

**书中对照**（归纳）：PDF 第 179～180 页：书中把 bind 称为给 socket 命名，并解释为什么服务端通常显式绑定而客户端往往交给系统分配。它举出受保护地址和地址占用两类失败。现代系统还受能力和命名空间相关设置影响，因此不要把“低端口只能由 root 绑定”写成无条件定律。

```c
int bind(int fd, const struct sockaddr *addr, socklen_t addrlen);
```

**传统学习重点**：“命名”就是为 socket 绑定本地地址，不是给变量起名字，也不是创建 DNS 域名。成功返回 0，失败返回 -1。

`127.0.0.1:9000` 只用于本机 IPv4 回环通信；`0.0.0.0:9000` 表示接受发往本机各 IPv4 本地地址的相应流量。`0.0.0.0` 不是客户端应该连接的远端地址。IPv6 的 `::` 为通配地址，其 IPv4 接收行为还受 `IPV6_V6ONLY` 等配置影响，不要假设始终双栈。

端口 0 请求内核分配可用端口，用 `getsockname` 取回，测试时可避免硬编码端口竞争。客户端通常不手动 `bind`，`connect` 时由内核选本地地址与临时端口。

`EADDRINUSE` 常表示绑定冲突，也可能涉及此前连接的地址复用规则；`EADDRNOTAVAIL` 常表示地址不是本机可用地址。绑定受限端口涉及系统策略和能力，本例使用普通高位端口或 0。

**现代写法**：IPv4 教学例用明确的 `sockaddr_in{}`；通用服务端用 `getaddrinfo(nullptr, service, AI_PASSIVE)` 的候选地址逐个尝试。若要设 `SO_REUSEADDR`，放在 `bind` 之前。C++ 不会改变端口冲突规则。

## 5.4 监听 socket

**书中对照**（归纳）：PDF 第 181～184 页，代码清单 5-3/5-4：书中让服务器只监听、不 accept，再通过多个客户端和连接状态观察 backlog，实验出现比配置值多一个的已建立连接。这个结果适合认识队列，却不是可移植的容量合同；队列满也不保证立即拒绝。本文采用现代 Linux 队列语义解释，不把某次实验数值当定律。

```c
int listen(int fd, int backlog);
```

**传统学习重点**：TCP socket 经 `listen` 成为被动监听端点。UDP 不执行这一过程。`backlog` 不是“允许一共接待多少客户”，也不是线程数量。

```text
SYN 到来 → 尚未完成握手的连接状态 → 完成握手的等待队列 → accept → 已连接 fd
                         listener 一直保留，供后续连接使用
```

Linux 的 `backlog` 主要限定等待 `accept` 的已建立连接队列，受 `somaxconn` 上限约束；未完成握手部分有另一套限制与机制，启用 SYN cookies 时还存在例外。握手主要由内核完成，不要求应用已经调用 `accept`。队列满时的可见行为受协议和配置影响，不应写死为“客户端必然立即报错”。参见 [listen(2)](https://man7.org/linux/man-pages/man2/listen.2.html)。

**现代写法**：明确配置 backlog，并让应用及时接受连接。更大的队列只能缓冲突发，无法解决业务处理能力不足。本教程的 16 是学习参数，不是生产调优结论。

## 5.5 接受连接

**书中对照**（归纳）：PDF 第 185～188 页，代码清单 5-5：书中故意延后 accept，让客户端先断网或退出，再观察 accept 仍可能成功，并看到 ESTABLISHED 或 CLOSE_WAIT。它说明 accept 不是探测客户端健康状态的 API；但不能扩大成“任何异常情况下 accept 都成功”，Linux 仍可返回待处理网络错误。

```c
int accept(int listener, struct sockaddr *peer, socklen_t *length);
```

成功返回一个**新的已连接 fd**，失败为 -1。原来的 listener 继续监听。新 fd 用来收发该客户端数据；不能把 listener 当业务连接去 `recv`。

如果不需要对端地址，可传两个空指针。阻塞 socket 没有可接受连接时等待；非阻塞则可能返回 `EAGAIN/EWOULDBLOCK`。`EINTR` 表示被信号中断，可按程序停止策略决定是否重试。

Linux 下普通 `accept` 返回的 fd 不继承 listener 的 `O_NONBLOCK`。现代 Linux 常用 `accept4(..., SOCK_CLOEXEC | SOCK_NONBLOCK)` 显式设置；本教程阻塞示例只设 `SOCK_CLOEXEC`。`accept4` 是 Linux 接口，在其他平台需要适配。参见 [accept(2)](https://man7.org/linux/man-pages/man2/accept.2.html)。

**OS 原理**：多个连接可共享同一服务端本地端口，内核依据协议与本地/远端地址、端口区分流。不是每接待一个客户就重新申请一个服务端监听端口。

**现代写法**：listener 与 peer 分别有明确所有者。长期服务还要处理临时网络错误、fd 耗尽、连接数限制和关闭请求。本例只服务一个客户端，方便观察完整生命周期。

## 5.6 发起连接

**书中对照**（归纳）：PDF 第 189 页：书中讲主动建立连接，重点解释拒绝和超时。本文保留这条主线，增加非阻塞 EINPROGRESS 的完成检查、候选地址重试，以及 UDP connect 与 TCP 握手的区别。

```c
int connect(int fd, const struct sockaddr *server, socklen_t length);
```

**传统学习重点**：TCP 客户端通常 `socket → connect → 收发 → close`。阻塞 `connect` 成功表示连接建立；不表示服务端应用已执行 `accept`，更不表示请求已完成。

拒绝连接常见 `ECONNREFUSED`，不可达可能是 `ENETUNREACH`，超时可能是 `ETIMEDOUT`。失败后重试其他候选地址时通常关闭旧 fd，创建新 socket。不要把失败 socket 的状态假设为完全恢复初始状态。

**非阻塞拓展**：返回 `-1/EINPROGRESS` 表示尚在进行，等待写就绪后读取 `getsockopt(fd, SOL_SOCKET, SO_ERROR, ...)`，只有得到 0 才能判定成功。写就绪本身也可能意味着失败已发生。需要期限时应配合事件等待与计时，不能仅无限等待。

**UDP 的 connect** 没有 TCP 握手，它设置默认对端，并限制通常接收的数据来源，使后续可以使用 `send/recv`；它不增加可靠性，也不证明远端正在运行。

**现代写法**：域名解析得到多个 IPv4/IPv6 候选，逐个新建 socket 尝试，保存每次错误。入门示例固定回环 IPv4，解析实验另行展示双栈结果。

## 5.7 关闭连接

**书中对照**（归纳）：PDF 第 190～191 页：书中通过 fork 后多份描述符解释 close 不一定立即结束连接，再引出 shutdown 的方向控制。更精确的理解是内核资源被多个描述符引用，而不是整数 fd 自身携带引用计数；shutdown 也不负责释放 fd。本文额外用半关闭串起请求和响应。

```c
int close(int fd);
int shutdown(int fd, int how);
```

**传统学习重点**：`close` 释放当前 fd；`shutdown` 改变 socket 的通信方向，但不释放 fd。

| 调用 | 行为 | 常见用途 |
|---|---|---|
| `shutdown(fd, SHUT_WR)` | 不再发送；正常 TCP 下已排队数据之后发送 FIN | 请求发送完毕，继续读响应 |
| `shutdown(fd, SHUT_RD)` | 关闭本端接收方向 | 不再接收，不宜作为通用业务确认 |
| `shutdown(fd, SHUT_RDWR)` | 两方向都关闭 | 停止通信，之后仍需 close |
| `close(fd)` | 释放这个描述符引用 | 资源回收 |

`shutdown` 影响底层 socket，因此其他引用也能观察到；`close` 一个副本可能并未断开连接。参见 [shutdown(2)](https://man7.org/linux/man-pages/man2/shutdown.2.html)。

### 半关闭为什么有用

客户端发完请求后 `shutdown(SHUT_WR)`，服务端读完已到达数据后 `recv` 返回 0，知道客户端发送方向结束；服务端仍可以发响应。客户端继续接收响应后才 `close`。如果双方都等待对方先关闭写方向，可能互相等住。

### FIN、RST、TIME_WAIT

FIN 表示某一方向正常结束；RST 表示连接被复位，数据可能未完成正常交付。主动正常关闭的一方通常经历 TIME_WAIT，帮助处理旧报文及末次确认重传。它属于 TCP 状态，不代表应用忘记 `close`。FIN_WAIT、CLOSE_WAIT 等也是内核协议状态：长时间大量 CLOSE_WAIT 常提示应用收到了 EOF 却未结束连接。

**现代写法**：析构负责兜底 `close`，但需要半关闭的业务过程显式调用 `shutdown`。析构函数不抛异常；Linux 下不要在 `close` 失败后盲目重试原编号，因为 fd 可能已释放并被复用。见 [close(2)](https://man7.org/linux/man-pages/man2/close.2.html)。

## 5.8 数据读写

**书中对照**（归纳）：PDF 第 192～200 页：书中从 read/write 扩展到 send/recv，借代码清单 5-6～5-8 演示发送多个紧急字节时只有最后一个按带外字节接收，再介绍 UDP 地址参数和 msghdr/iovec。本文另加完整读写、长度前缀、合法空 UDP 报文和截断处理。不要照搬固定三次 recv 的实验结构作为通用协议解析器，也不要把 msg_flags 当发送 flags 的输入位置。

### 5.8.1 TCP 数据读写

```c
ssize_t send(int fd, const void *buffer, size_t length, int flags);
ssize_t recv(int fd, void *buffer, size_t capacity, int flags);
```

**传统学习重点**：返回值才是实际处理的字节数。发送缓冲区空间、当前可读数据、信号等都可能导致短读/短写；不能假定一次完成全部。

| 返回情况 | `send` | `recv`（请求容量大于 0 的 TCP） |
|---|---|---|
| 正数 n | 接受 n 字节供后续发送 | 收到 n 字节 |
| 0 | 不应让发送循环原地空转 | 对方发送方向结束，队列已读完 |
| -1 | 检查 errno | 检查 errno |

`EINTR` 处理后可重新尝试；`EAGAIN/EWOULDBLOCK` 表示现在不能继续，非阻塞程序应该等待就绪，而不是忙循环。`ECONNRESET` 表示复位。对已关闭写方向的连接发送可能产生 `EPIPE` 和 SIGPIPE；Linux 下 `MSG_NOSIGNAL` 禁止本次发送触发该信号，但仍须处理错误返回。发送接口见 [send(2)](https://man7.org/linux/man-pages/man2/send.2.html)。

`recv(fd, buf, 0, ...)` 返回 0 不能据此判定 EOF，因此表格明确要求容量大于 0。`MSG_PEEK` 查看但不消费数据；`MSG_WAITALL` 尝试等足所需长度，但信号、错误或 EOF 等仍能导致提前返回。接收接口见 [recv(2)](https://man7.org/linux/man-pages/man2/recv.2.html)。

### TCP 的消息边界需要自己定义

客户端依次发送 `hello`、`world`，服务端可能先得到 `he`，再得到 `lloworld`；也可能一次得到 `helloworld`。这不是数据错乱，而是 TCP 从未承诺保留每次 send 的分组。

常见应用层协议：固定长度、分隔符（如行结束符）、长度前缀。本教程使用：

```text
4 字节无符号大端长度 N | 紧接 N 字节正文
00 00 00 05            | h e l l o
```

接收端先循环读满 4 字节，解码长度，检查最大值（本例 1 MiB），再循环读满正文。第一个头字节之前 EOF 表示正常结束；头/正文中途 EOF 表示截断。N 为 0 是合法空消息，不等于断开。长度上限防止对方声称超大消息导致分配失控。

不能直接发送整个 C++ 对象：对象可能包含指针、填充字节、本机字节序和不稳定布局。序列化必须定义字段顺序、宽度和编码。二进制内容也可能含 `\0`，所以接收后不能依赖 `strlen` 或 `%s`。

### 5.8.2 UDP 数据读写

```c
ssize_t sendto(int fd, const void *buf, size_t n, int flags,
               const struct sockaddr *to, socklen_t tolen);
ssize_t recvfrom(int fd, void *buf, size_t n, int flags,
                 struct sockaddr *from, socklen_t *fromlen);
```

**传统学习重点**：发送时带目的地址，接收时得到来源地址。每次发送一份数据报；一次接收最多消费一份。缓冲区不足时数据报的剩余部分会丢弃，不能靠下一次 recv 补回来。零长度数据报合法，返回 0 不表示对端退出。

UDP 不应套用 TCP 的 `send_all`：把“剩余部分”再发送会形成另一个数据报，改变协议。发送过大可能 `EMSGSIZE`；IPv4 UDP 的理论载荷上限 65507 字节不等于实用推荐大小，实际还要考虑路径 MTU、IP 分片与丢包。

**现代写法**：明确协议最大数据报长度，必要时用 `recvmsg` 检查 `MSG_TRUNC`。设置接收期限；若业务需要可靠性，还要定义编号、确认、重传和去重，或选择现成传输协议。UDP echo 只是演示，不具备这些保证。

### 5.8.3 通用数据读写函数

`read/write` 可操作 socket，也可操作文件/管道，但没有 socket 专属 flags；对 socket 用 `write` 时也需考虑 SIGPIPE。更通用的 socket 接口是 `sendmsg/recvmsg`：

```cpp
char header[] = "HEAD:";
char body[] = "body";
iovec chunks[] = {{header, 5}, {body, 4}};
msghdr message{};
message.msg_iov = chunks;
message.msg_iovlen = 2;
// 对未 connect 的 UDP，还要设置 msg_name 和 msg_namelen。
```

`iovec` 的每项是“地址+长度”。发送时 gather（汇集）多段缓冲区，接收时 scatter（分散）写入多个缓冲区。示例一次 UDP `sendmsg` 发送的是一个 9 字节数据报，并非两个报文。TCP 中它仍然是字节流，并且可能短写，必须跨 iovec 推进偏移。

| msghdr 字段 | 用途 |
|---|---|
| `msg_name / msg_namelen` | 对端地址及长度 |
| `msg_iov / msg_iovlen` | 缓冲区数组及项数 |
| `msg_control / msg_controllen` | 辅助数据缓冲区，如时间戳、UNIX 域 fd 传递 |
| `msg_flags` | recvmsg 输出的状态，例如数据截断 |

不要从书中手抄 msghdr 的结构体定义：当前 glibc 的部分成员类型与书中展示的布局不同，应始终包含系统头文件并使用其中的声明。对于已连接 TCP，若用 `sendto` 忽略地址，尾部参数写 `nullptr, 0`（C 写 `NULL, 0`），因为最后一个参数是长度整数；`recvfrom` 则是地址指针和长度指针，可写两个空指针。参见 [send(2)](https://man7.org/linux/man-pages/man2/send.2.html)。

发送 flags 通过 `sendmsg` 的第三个参数传入，不是写在 `msg_flags` 中。接收时辅助数据要检查 `MSG_CTRUNC`，并用 `CMSG_*` 宏遍历，不能靠猜测对齐方式手工挪指针。重复接收前重设长度等输入字段。辅助数据不是应用正文的“隐藏头部”。

**现代写法**：用 `std::array<iovec, N>` 管数组，用拥有者维持各缓冲区生命周期。减少应用拼接复制不代表完整“零拷贝”；普通内核数据路径仍可能复制。配套 `api_lab msg` 故意接收过小，演示截断。

## 5.9 带外标记

**书中对照**（归纳）：PDF 第 201 页：书中把 I/O 复用异常事件和 SIGURG 作为紧急数据通知方式，用 sockatmark 定位读取边界。本文选择 SO_OOBINLINE 实验，因此到标记处仍按普通 recv 读取；书中的 MSG_OOB 读取应与其非 inline 前提一起理解。

**传统学习重点**：TCP 紧急数据机制、`MSG_OOB`、`SO_OOBINLINE` 与 `sockatmark` 的关系。这里“带外”不是第二条独立 TCP 连接。

Linux TCP 的紧急数据机制通常只提供一个字节的紧急数据语义，新的紧急指针还能改变之前的状态，不能把它当可靠的多消息优先队列。未启用 `SO_OOBINLINE` 时，紧急字节通常用 `recv(..., MSG_OOB)` 读取；启用后，它在普通输入流中读取。参见 [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html)。

```c
int sockatmark(int fd); // 1：读取位置在标记处；0：不在；-1：错误
```

它检查**当前读取位置**是否到标记，不是在询问“是否存在任意待处理紧急事件”。可以通过 `poll` 的 `POLLPRI` 或 `select` 异常集合观察紧急状态；就绪与读取位置仍是两个概念。参见 [sockatmark(3)](https://man7.org/linux/man-pages/man3/sockatmark.3.html)。

如果选择书中提到的 SIGURG 路线，还需要通过 `fcntl(F_SETOWN)` 等设置 socket 的信号接收者，并设计适当的信号处理；仅安装处理函数并不足以把该 socket 的紧急通知定向给进程。本例用事件等待，不使用信号。参见 [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html)。

实验 `api_lab oob` 创建本机 TCP 连接，发送 `abc`、一个带 `MSG_OOB` 的 `!`、再发送 `xyz`，接收端启用 inline 并逐字节打印标记。逐字节仅为教学，真实程序不能为了检测标记长期这样低效读取。

**现代实践**：新业务通常在应用协议中定义消息类型、优先级和控制消息，避免依赖这套历史紧急数据机制；这不代表 C++20 删除了 OOB API。

## 5.10 地址信息函数

**书中对照**（归纳）：PDF 第 202 页：书中成对介绍 getsockname/getpeername，并说明输出地址空间不足会截断。本文用动态端口查询和客户端端点打印说明其用途，并提醒它们返回的是直接连接端点，不是代理背后的最终身份。

```c
int getsockname(int fd, struct sockaddr *local, socklen_t *length);
int getpeername(int fd, struct sockaddr *remote, socklen_t *length);
```

**传统学习重点**：前者查本地端点，后者查已关联的对端。两者都用输入容量/输出长度的参数形式；失败返回 -1。

服务端 `bind` 端口 0 后通过 `getsockname` 打印实际端口。客户端连接后也能查询自己自动分配的本地端口。`accept` 时已返回过对端地址，并不妨碍以后用 `getpeername` 再查。

监听 socket 没有单一对端；未 connect 的 UDP 也没有默认对端，此时 `getpeername` 可能 `ENOTCONN`。通过代理访问时这里得到的是实际直接连接端点，不一定是最终用户身份；NAT 也会影响观察到的地址。

**现代写法**：用 `sockaddr_storage` 接收结果，再交给 `getnameinfo` 做统一的 IPv4/IPv6 文本格式化。日志常用数字地址标志避免无意触发反向名称查询。

## 5.11 socket 选项

**书中对照**（归纳）：PDF 第 203～213 页：书中先解释 level、值类型和设置时机，再讨论四组选项。代码清单 5-9 演示地址复用，5-10/5-11 修改缓冲区并结合抓包观察接收窗口。握手协商相关配置应提前设置，但不能推导成所有选项都只能在 connect/listen 前设置；各选项应按各自规则使用。旧实验中的缓冲区最小值也不应当作当前固定值。

```c
int setsockopt(int fd, int level, int name, const void *value, socklen_t length);
int getsockopt(int fd, int level, int name, void *value, socklen_t *length);
```

`level` 表示选项属于哪一层：`SOL_SOCKET` 为通用 socket 层，`IPPROTO_TCP` 为 TCP。值可以是整数，也可以是 `linger/timeval` 等结构体，必须按具体选项选择类型和大小。

```cpp
int enabled = 1;
check(::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)), "reuseaddr");
```

### 5.11.1 SO_REUSEADDR

**传统学习重点**：在 `bind` 前设置，用于放宽特定的本地地址复用条件，帮助服务器重启场景。它不是“任意两个活动服务都能占同一地址端口”的开关，也不会消除 TIME_WAIT。Linux 的复用规则还涉及旧、新 socket 的设置。

**现代实践**：区分 `SO_REUSEPORT`，后者在符合条件时允许多个 socket 绑定同一地址端口，常用于多个监听者分担连接。不要为了隐藏占用错误无条件加入两者。

书中还提到 `tcp_tw_recycle`。这是历史配置，Linux 4.12 已移除，不能作为现代服务器的优化步骤；复用地址也不等于消除 TIME_WAIT。见 [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html)。

### 5.11.2 SO_RCVBUF 和 SO_SNDBUF

分别影响内核收发缓冲区预算，区别于用户代码的 `char buf[4096]`。Linux 会为管理开销将设置值加倍，查询可见加倍后的数值，并受系统限制；不能保证请求什么就精确得到什么。TCP 还有自动调节机制。

**OS 补充**：应用接收慢会积压接收队列，TCP 接收窗口可能缩小；发送端排队过多造成延迟和内存压力。缓冲区越大不等于越快，高连接数时每连接的资源消耗必须一起评估。

### 5.11.3 SO_RCVLOWAT 和 SO_SNDLOWAT

低水位用于描述相关操作/就绪判断的最低数据阈值。Linux 支持设置接收低水位；现代 Linux 的 select/poll/epoll 就绪判断会考虑它。Linux 不支持调整发送低水位，尝试设置可能 `ENOPROTOOPT`。

不能用接收低水位代替协议分帧，也不能据此不处理短读、EOF、错误或非阻塞情况。本例只查询默认低水位，不假装发送低水位可以调节。

### 5.11.4 SO_LINGER

```c
struct linger { int l_onoff; int l_linger; };
```

| 设置 | 理解重点 |
|---|---|
| `l_onoff = 0` | 默认关闭 linger；close 通常快速返回，内核继续处理正常关闭 |
| 开启且超时为正 | 关闭可能等待已排队数据处理或期限到达，不适合随意放进事件循环 |
| 开启且超时为 0 | TCP 常用作中止式关闭，丢弃待发数据并复位连接 |

这不是“对方业务已处理”的确认机制。关闭行为还受连接状态、未读数据等影响。现代项目通常先用应用层确认与明确关闭流程，只有理解需求后才设置 linger。以上选项细节参见 [socket(7)](https://man7.org/linux/man-pages/man7/socket.7.html)。

### 相关但不同的选项

`TCP_NODELAY` 关闭 Nagle 算法，并不保证立即到达或取消所有缓冲；`SO_KEEPALIVE` 提供内核探测机制，不替代业务心跳和请求超时；`SO_RCVTIMEO/SO_SNDTIMEO` 影响阻塞 I/O 等待，不等于给整次请求设置总期限，也不直接控制 epoll 等待时间。每个选项都应有明确目标，再测量收益。

## 5.12 网络信息 API

**书中对照**（归纳）：PDF 第 214～222 页：书中先介绍旧 hostent/servent 接口，用代码清单 5-12 访问 daytime 服务，再转到 getaddrinfo、结果释放、getnameinfo 与专属错误码。本文改为无外部服务依赖的 localhost/数字地址实验。书中 read 满缓冲区后再追加零字符的写法有越界风险，现代代码按实际长度输出，或明确预留一个字节。

### 5.12.1 gethostbyname 和 gethostbyaddr

**传统学习重点**：名称与主机地址之间的查询。`gethostbyname` 返回 `hostent*`，包含正式名称、别名、地址类型、地址长度和地址列表。经典接口以 IPv4 为主；`gethostbyaddr` 根据二进制地址查询名字。

旧函数可能返回静态存储，不能由调用者 free，也不适合把返回指针长期保留或跨并发查询使用。错误传统上通过 `h_errno/hstrerror`，不是简单套 `perror`。

**现代实践**：使用 `getaddrinfo/getnameinfo` 统一处理 IPv4/IPv6，明确结果所有权。名称查询不一定走 DNS：本机 hosts 文件、NSS 配置等也参与，因此 `localhost` 查询不必联网。

### 5.12.2 getservbyname 和 getservbyport

服务名例如 `http`，协议名例如 `tcp`，可以查出服务端口；反向查询从端口得到服务名。结果 `servent` 中的 `s_port` 是网络字节序，打印前 `ntohs`。`getservbyport` 的输入端口也要网络字节序。

这些函数查询服务数据库（常见 `/etc/services` 等来源），不是扫描网络。查到 `http/tcp = 80` 不代表本机 80 端口存在 HTTP 服务。结果同样有静态存储方面的限制；实验只顺序查询并立即使用。

### 5.12.3 getaddrinfo

```cpp
addrinfo hints{};
hints.ai_family = AF_UNSPEC;       // 接受 IPv4 或 IPv6 候选
hints.ai_socktype = SOCK_STREAM;   // 指定流式服务，减少不相关结果
addrinfo* result = nullptr;
int rc = ::getaddrinfo("localhost", "80", &hints, &result);
```

**传统学习重点**：同时处理主机名与服务名，返回链表；每个节点都提供创建 socket 和 connect/bind 所需的地址族、类型、协议、地址及长度。`ai_next` 指向下一个节点，空指针表示结束。成功返回 0，失败用 `gai_strerror`。

服务端可用 `AI_PASSIVE` 配合空主机名获取通配绑定地址。仅接受数字输入时用 `AI_NUMERICHOST/AI_NUMERICSERV`。C++ 使用带 `freeaddrinfo` 删除器的 `unique_ptr` 统一释放整条链表，不对每个 `ai_addr` 单独 free。参见 [getaddrinfo(3)](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html)。

**现代实践**：不能只拿第一条结果就认定域名不可用，要尝试其他候选。同步解析可能阻塞，事件循环内需要调度到适当执行环境或使用异步解析器。RAII 解决内存释放，不会让解析自动异步。

当前 Linux man-pages 将 getaddrinfo 标注为 `MT-Safe env locale`；不要把书中关于内部是否调用 `_r` 函数的说明当成今天判断线程安全的依据，仍应遵守环境与 locale 的并发使用约束。

### 5.12.4 getnameinfo

它把 socket 地址转换成主机名/服务名或数字字符串。`NI_NUMERICHOST | NI_NUMERICSERV` 强制数字输出，例如 `127.0.0.1` 和 `80`，适合低开销日志；不设置数字标志时可能进行名称查询。`NI_NAMEREQD` 要求成功得到名称，得不到则失败。

同时提供输出缓冲区与容量，检查返回错误码；IPv6 显示端点时通常写 `[::1]:80`，避免冒号歧义。即使反查成功，域名也不是可信身份凭据。参见 [getnameinfo(3)](https://man7.org/linux/man-pages/man3/getnameinfo.3.html)。

## 6. 从基础 API 到现代高性能服务器

### 6.1 C++20 不会替换 Linux 协议栈

变化主要在资源管理和程序组织：C 的手动 close 变成 RAII，裸缓冲区变成有大小的视图，错误码可封装为返回类型或异常，复杂回调可由协程表达。但就绪通知、超时、取消、消息长度限制和背压仍需实现。

协程 `co_await` 是可挂起计算的语言机制；必须有运行时或库把 socket 就绪事件与协程恢复关联。仅写协程不会令阻塞 `recv` 自动变非阻塞。C++20 标准范围可查 [WG21 N4861 工作草案](https://timsong-cpp.github.io/cppwp/n4861/)；本教程不依赖第三方网络库。

### 6.2 从单连接例子扩展的顺序

1. 先正确处理一条连接：短读写、分帧、EOF、半关闭、异常清理。
2. 再处理多个连接：维护每连接的接收缓存、解析进度和待发队列。
3. 将 socket 设置非阻塞，用 poll/epoll 等待就绪；读写至 `EAGAIN` 后保存进度。
4. 加入超时、限流、最大连接数、每连接缓存上限；慢客户端不能无限占内存。
5. 最后基于测量选择线程池、批量 I/O、内存复用和更复杂的传输方案。

就绪通知不是“操作已经替你做完”。边沿触发下如果未读到 `EAGAIN` 就停止，可能遗漏后续处理机会。关闭前还要协调事件注册、对象生命周期和 fd 重用，RAII 不是并发安全的完整方案。

### 6.3 为什么背压很重要

假设业务每秒产生 10 MiB 响应，而客户端每秒只读 1 MiB，多出来的数据必须排队。若应用不限制队列，即使内核发送缓冲区有限，用户态队列也可能无限增长。背压就是在下游跟不上时限制上游继续生产，例如暂停读取新请求、限制待发队列或结束长期过慢连接。

本教程阻塞写在一定程度上让发送线程等待，但会被慢对端占住，因此它是基础 API 实验，不是高并发成品。回环测试也无法模拟真实网络丢包、拥塞、延迟和大量连接竞争。

## 7. 调试与复习

运行命令、预期输出和各程序细节见[配套 readme](../code/base_linux_net_api/readme.md)。有相关工具时可以执行：

```bash
ss -lnt                 # TCP 监听端点
ss -nt                  # 已建立等 TCP 状态
ss -lun                 # UDP 端点
strace -e trace=network,close ./tcp_cpp20 server 9000
```

`man 2 socket` 中 2 是手册分类（系统调用），`man 3 getaddrinfo` 中 3 是库函数分类，不是 API 版本。

| 现象 | 优先排查 |
|---|---|
| bind 失败 | 地址是否属于本机、端口是否冲突、权限与复用设置 |
| connect 失败 | 进程是否监听、IP/端口是否匹配、地址族与网络策略 |
| recv 一直等 | 对方是否发送、协议长度是否正确、是否忘记半关闭 |
| 少收到数据 | 是否把一次 recv 当完整消息、是否忽略实际返回长度 |
| UDP 超时 | 是否丢包、服务端是否已退出、是否使用了错误地址 |
| 进程突然退出 | 是否发生 SIGPIPE、未捕获异常或其他错误 |
| fd 越来越多 | 每条成功/失败路径是否都释放了所有权 |

复习时尝试回答：为什么 listener 与 peer 是两个 fd？为什么 TCP 的两个 send 不能当两条消息？为什么 UDP 的零字节和 TCP EOF 不一样？为什么长度前缀要限制上限？为什么 `std::span` 不保证异步内存存活？为什么 `send` 成功不能证明业务成功？

答案贯穿本文：协议定义语义，内核管理通信状态，应用定义消息与业务确认，C++ 管理对象和资源生命周期。把这四个层次分清，比背下函数名更重要。

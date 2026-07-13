# 网络字节序——为什么你的消息接收端读出来是乱码

> **目标读者**：写过 C/S 程序，发现收到的 int 是"反的"；见过 `htonl`、`ntohs` 但不清楚什么时候用；想理解字节序在协议设计和消息队列中的影响。

---

## 目录

1. [什么是字节序](#1-什么是字节序)
2. [大端 vs 小端](#2-大端-vs-小端两个世界的故事)
3. [网络字节序](#3-网络字节序tcpip的约定)
4. [四个必用函数](#4-四个必用的转换函数)
5. [实战协议头](#5-实战协议头中的字节序处理)
6. [消息队列中的字节序](#6-消息队列中的字节序问题)
7. [示例代码说明](#7-完整示例代码说明)
8. [总结](#8-总结)

---

## 1. 什么是字节序

### 1.1 从一个乱码开始

```cpp
int value = 16909060;  // 0x01020304
send(fd, &value, sizeof(value), 0);
// 服务器收到：
int received;
recv(fd, &received, sizeof(received), 0);
printf("%d\n", received);
```

如果 C/S 都是 x86，打印 `16909060`。如果服务器是 PowerPC（大端），打印 `61680`（0x04030201）。数据没丢，字节顺序反了。

### 1.2 什么是字节序

`0x01020304` 在内存中的布局：

```
          地址+0  +1  +2  +3
小端(x86):  04   03   02   01    ← 低位在低地址
大端(Power): 01   02   03   04    ← 高位在低地址
```

- **小端 (Little-Endian)**：x86/x64、ARM（默认）、手机、PC
- **大端 (Big-Endian)**：PowerPC、网络协议标准、某些嵌入式

字符串("ABCD")不受字节序影响。**受影响的都是多字节数值**：short、int、long、float。

### 1.3 检测自己机器的字节序

```cpp
uint32_t test = 0x01020304;
auto* bytes = (const uint8_t*)&test;
if (bytes[0] == 0x04) printf("小端\n");
else if (bytes[0] == 0x01) printf("大端\n");

// C++20:
if constexpr (std::endian::native == std::endian::little) // 小端
```

---

## 2. 大端 vs 小端——两个世界的故事

```
整数 0x12345678:

大端: [0x12][0x34][0x56][0x78]  ← 人类阅读习惯
小端: [0x78][0x56][0x34][0x12]  ← CPU 计算方便

字符串 "ABCD" 两种都存为 ['A']['B']['C']['D'] — 不受影响
```

为什么有两种？大端符合人类阅读从左到右的习惯。小端方便 CPU 从低位开始做加法。x86 选了小端，网络协议选了大端。**两种都活下来了**。

---

## 3. 网络字节序——TCP/IP 的约定

### 3.1 统一大端

TCP/IP 规定：**网络传输用大端序（Big-Endian）**，也叫网络字节序。

```
小端发 0x01020304:
  内存存 04 03 02 01 → htonl() 转 → 04 03 02 01 不变? 
  
  不对！
  
  小端内存:   04 03 02 01   (值=0x01020304)
  htonl转后:  01 02 03 04   (网络字节序=大端)
  
  大端收到:   01 02 03 04   (网络值=0x01020304)
  ntohl转后:  04 03 02 01   (转回主机字节序)
  
  最终值 = 0x01020304 ✔
```

### 3.2 哪些要转哪些不用

| 必须转 | 不需要转 |
|--------|---------|
| 端口号 (uint16_t) | char/uint8_t 单字节 |
| IP 地址 (uint32_t) | 字符串 |
| 消息长度字段 (uint32_t) | JSON/XML/文本 |
| 消息类型 ID (uint16_t) | 已序列化的字节流 |
| 协议头中的数值字段 | struct 中的填充(padding) |

**应用层协议里的数值，你自己负责转。** IP/TCP 头里的转换是内核帮你做的，但你的消息体里写了 int，内核不知道那是 int，你得自己调 htonl/ntohl。

---

## 4. 四个必用的转换函数

### 4.1 速查表

| 函数 | 全称 | 方向 | 类型 | 用途 |
|------|------|------|------|------|
| `htons()` | Host To Network Short | 发前转 | uint16_t | 端口号、类型 ID |
| `htonl()` | Host To Network Long  | 发前转 | uint32_t | IP 地址、长度 |
| `ntohs()` | Network To Host Short | 收后转 | uint16_t | 收到的端口/类型 |
| `ntohl()` | Network To Host Long  | 收后转 | uint32_t | 收到的长度/IP |

记法：`h→n`=发前转(主机到网络)，`n→h`=收后转，`s`=16位，`l`=32位。

### 4.2 ⚠️ 补充：不要用 boost::asio::detail 中的字节序函数

网上一些博客会教你这么写：

```cpp
uint32_t net = boost::asio::detail::socket_ops::host_to_network_long(val);
```

**这不是企业做法。** 原因：

1. **`detail` 命名空间是 ASIO 内部实现**——ASIO 版本升级时可能改名、改参数、甚至删除，不提供向后兼容
2. **不需要绕一圈**——ASIO 内部也只是调了 `htonl`，你直接调 `htonl` 省掉一层包装
3. **`detail` 代码可读性差**——同事看到 `detail::` 会问"为什么不用标准函数"

```cpp
// 这三者最终干的事一样：
uint32_t a = htonl(val);                                                // ✅ 标准 POSIX
uint32_t b = boost::asio::detail::socket_ops::host_to_network_long(val); // ❌ 内部细节
uint32_t c = boost::asio::detail::socket_ops::host_to_network_long(val); // ❌ 换版本可能编译不过
```

**企业习惯**：`htonl` / `htons` / `ntohl` / `ntohs` 四个函数写遍所有平台，有且只有这四个。

### 4.2 什么时候调用

```cpp
// 发送端
uint32_t body_len = msg.body.size();
uint32_t net_len = htonl(body_len);  // ← 发之前转
send(fd, &net_len, 4, 0);            // 发的已经是网络字节序
send(fd, msg.body.data(), body_len, 0); // body 是二进制或字符串，不转

// 接收端
uint32_t net_len;
recv(fd, &net_len, 4, 0);            // 收上来的是网络字节序
uint32_t body_len = ntohl(net_len);   // ← 收之后转
// body_len 现在可以在本机用了
```

### 4.3 常见错误

```cpp
// ❌ 错误：收到的值没转直接用了
uint32_t len;
recv(fd, &len, 4, 0);
auto* buf = new char[len];   // len 还是网络字节序！
// 如果 len=0x00000100(网络)，本机可能读到 0x01000000=16777216

// ❌ 错误：字节序转换发生在 copy 之后
uint32_t len;
recv(fd, &len, 4, 0);
uint32_t host_len = ntohl(len);
memcpy(&msg.header.len, &len, 4);   // ← 存了网络字节序的 len！
// 以后每次读 msg.header.len 都要 ntohl，容易忘

// ✅ 正确：存主机字节序
msg.header.len = ntohl(len);         // 已经转好了
// 以后直接用 msg.header.len
```

### 4.4 大端机器上的情况

如果机器本身就是大端，htons/htonl/ntohs/ntohl 是**空操作**（宏定义为什么都不做）：

```c
// 大端机器上 htonl 的实现：
#define htonl(x) (x)   // 什么都不做！
```

因为主机字节序 = 网络字节序（都是大端），不需要转换。**但你仍然要调用它们**——保证代码跨平台可移植。不要自己写 `if (is_little_endian) { swap; }`，用标准函数就好。

---

## 5. 实战——协议头中的字节序处理

### 5.1 TLV 协议头的字节序

```
[ Type: 2B ][ Length: 4B ][ Payload: N B ]
   uint16_t     uint32_t
   要转 htons    要转 htonl
```

```cpp
// ===== 发送端 =====
struct MsgHeader {
    uint16_t type;    // 本地用（主机字节序）
    uint32_t len;     // 本地用（主机字节序）
};

void send_message(tcp::socket& sock, uint16_t type,
                  const std::vector<char>& body) {
    // 发送时转网络字节序
    uint16_t net_type = htons(type);
    uint32_t net_len  = htonl(body.size());

    // 发送：先发 type，再发 len，再发 body
    std::array<boost::asio::const_buffer, 3> bufs = {
        boost::asio::buffer(&net_type, 2),
        boost::asio::buffer(&net_len, 4),
        boost::asio::buffer(body)
    };
    boost::asio::write(sock, bufs);
}

// ===== 接收端 =====
struct IncomingHeader {
    uint16_t type;    // 网络字节序（收到时）
    uint32_t len;     // 网络字节序（收到时）
};

// 读 header 回调
void on_header_read(tcp::socket& sock, IncomingHeader hdr) {
    // 收到后立即转为主机字节序
    uint16_t type = ntohs(hdr.type);
    uint32_t len  = ntohl(hdr.len);

    if (len > MAX_BODY_SIZE) {
        close(sock, "body too large");
        return;
    }

    // 后续代码只用主机字节序的 type 和 len
    // type 和 len 已经安全了
    read_body(sock, type, len);
}
```

### 5.2 序列化结构体的陷阱

```cpp
// ❌ 错误：直接把 struct 整个 send
struct Packet {
    uint32_t id;
    uint16_t cmd;
    uint32_t body_len;
    char     data[1024];
};

Packet pkt;
pkt.id = htonl(12345);
pkt.cmd = htons(1);
pkt.body_len = htonl(actual_len);
memcpy(pkt.data, msg, actual_len);

send(fd, &pkt, sizeof(pkt), 0);  // ← 问题：pkt 有内存填充(padding)！
```

`struct` 里的内存对齐（padding）在不同编译器和平台上不一样。x86 上 `sizeof(Packet)` 可能是 1036，ARM 上可能是 1040。

**正确做法**：逐字段序列化，不用 struct 直接 send：

```cpp
// ✅ 正确：逐字段序列化
uint32_t net_id = htonl(12345);
uint16_t net_cmd = htons(1);
uint32_t net_len = htonl(body.size());

std::array<boost::asio::const_buffer, 4> bufs = {
    boost::asio::buffer(&net_id, 4),
    boost::asio::buffer(&net_cmd, 2),
    boost::asio::buffer(&net_len, 4),
    boost::asio::buffer(body)
};
boost::asio::write(sock, bufs);
```

### 5.3 端口号的字节序

```cpp
// 你平时写：
sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);    // ← 端口号要转网络字节序
addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
// inet_addr 返回的已经是网络字节序！
// 因为它内部调用了 htonl 或者直接按大端解析的

// 或者用现代写法：
addr.sin_port = htons(8080);
inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
// inet_pton 返回的 s_addr 也是网络字节序
```

**规律总结**：所有进入 `sockaddr_in` 的数值都是网络字节序（sin_port、sin_addr），所有从 `sockaddr_in` 读出的数值都要转回主机字节序（`getsockname`、`accept`、`recvfrom` 过来的地址）。

---

## 6. 消息队列中的字节序问题

### 6.1 问题一：消息体里的数值

消息队列里存的消息体（byte array）是在发送端序列化的。序列化时如果用了 `htonl`，那**入队时已经转成了网络字节序**。

```
发送端:
  msg.id = htonl(42)        → 转网络字节序
  msg.body = assemble()
  msg_queue.push(msg)       ← 队列里存的是网络字节序数据

  队列                      ← 数据在网络字节序状态
  
接收端:
  msg = msg_queue.pop()     ← 拿到的也是网络字节序
  id = ntohl(msg.id)        ← 转回主机字节序使用
```

**设计原则**：队列里存的是序列化后的字节流（网络字节序），每个消费者拿到后自己转主机字节序。这样队列不关心字节序，收发端各管各的。

### 6.2 问题二：队列元数据的字节序

如果消息队列本身有多字节元数据（如消息长度、优先级、时间戳），这些字段也存在字节序问题——但它们**只在队列内部使用**，不走网络，所以不需要转：

```cpp
struct QueueMessage {
    uint32_t    length;    // 队列内部使用的主机字节序
    uint8_t     priority;  // 单字节，不受影响
    uint64_t    timestamp; // 队列内部使用的主机字节序
    std::vector<char> data; // 已序列化的数据（含网络字节序的字段）
};
```

**原则**：数据跨机器时转（网络传输），不跨机器时不转（队列内部）。

### 6.3 问题三：序列化一次，到处使用

如果在消息队列里同时有多个消费者（多线程），它们看到的都是同一份序列化数据。如果数据是网络字节序，所有消费者都得 ntohl。如果数据是主机字节序，发送端就得先转好再入队。

**通用做法**：

```
发送端序列化（已排好字节序）
  │
  ▼
[消息队列]    ← 字节流不清真也不假，原样存储
  │
  ▼
接收端反序列化（自己转字节序）
```

实际上更常见的做法是：**发送端序列化时统一用网络字节序，数据进队列，接收端反序列化时统一转主机字节序**。这样队列本身不关心字节序，消息可以安全地从任何架构的机器发送到任何架构的机器。

---

## 7. 完整示例代码说明

本章节的配套代码在 `code/cpp_net/base_byteorder/` 目录下：

| 文件 | 作用 | 端口 |
|------|------|------|
| `server.cpp` | TLV 协议 echo server | 9190 |
| `client.cpp` | TLV 协议 echo client | 连接 9190 |

### 7.1 协议格式

```
[ Type: 2B ][ BodyLen: 4B ][ Body: N B ]
   htons        htonl        原始数据
```

所有多字节字段都转为网络字节序发送/接收。

### 7.2 io.run() 是什么——ASIO 的事件循环引擎

这是 ASIO 里你必须搞懂的一个概念。server.cpp 里有两段关键的代码：

```cpp
asio::io_context io;  // ① 创建事件循环
// ...
io.run();             // ② 启动事件循环（阻塞当前线程）
```

**`io_context` 就像一个大堂前台**。你（代码）把要做的事告诉前台：

```cpp
// "有客户端连接了叫我"
acceptor.async_accept(sock, callback);
// "数据读完了叫我"
asio::async_read(sock, buf, callback);
```

前台把这些事记在本子上，然后一句 `io.run()`，前台就开始工作了。

**`io.run()` 启动后内部做了什么：**

```
io.run()
    │
    └── while (还有未完成的任务) {
        │
        ├── 等事件 → epoll_wait(fds, ...)
        │   （没有事件时线程阻塞在这，不占 CPU）
        │
        └── 事件来了 → 执行对应的 callback
            │
            ├── async_accept 的 callback 执行了
            │   → 里面又注册了下一轮 async_accept
            │   → 也调用了 async_read（注册新的"待办事项"）
            │
            ├── async_read 的 callback 执行了
            │   → 里面又注册了下一轮 async_read
            │
            └── 回到循环顶部，继续等事件
    }
```

**关键理解**：

| 你经常看到的 | 实际上 |
|-------------|--------|
| `io_context io;` 创建对象 | 创建"前台"和"待办清单" |
| 调用 `async_accept` / `async_read` | 往待办清单上写了一条 |
| 调用 `io.run()` | 前台开始工作——**阻塞在这里，永不返回**（除非所有任务完成或出错） |

**`io.run()` 阻塞的是当前线程。** 如果写 `io.run()` 之后又写 `std::cout << "done"`，这行 cout **永远不会执行**——除非所有连接断完了。

server.cpp 里的多线程写法：

```cpp
for (int i = 1; i < thread_count; ++i) {
    threads.emplace_back([&io] { io.run(); });  // 额外线程跑前台
}
io.run();  // 主线程也来跑前台
// 多个线程同时跑同一个 io_context 的前台
// 一个事件来了，谁空闲谁处理
```

**一句话**：`io.run()` = 启动事件循环，开始监听和处理所有异步操作，阻塞当前线程直到无事可做。

### 7.3 代码亮点

- 全部用 ASIO 实现（现代 C++ 网络编程）
- 同步接口，便于理解字节序处理逻辑
- 完整处理了粘包（先读 6 字节 header，再读 body）
- 错误处理：非法长度、连接断开
- echo 服务：收到什么发回什么

编译运行：

```bash
cd code/cpp_net/base_byteorder
g++ -std=c++17 server.cpp -lboost_system -lpthread -o server
g++ -std=c++17 client.cpp -lboost_system -lpthread -o client
./server &
./client
```

---

## 8. 总结

### 8.1 铁律三条

```
1. 发之前调 htons/htonl，收之后调 ntohs/ntohl
2. 应用层协议的每个多字节字段都要转，不是只转第一个
3. 不要把 struct 整个 send——用逐字段序列化
```

### 8.2 速查卡

| 场景 | 处理 |
|------|------|
| 填写 sockaddr_in 的端口 | `htons(port)` |
| 填写 sockaddr_in 的 IP | `inet_addr()` 或 `inet_pton()` 返回已经是网络字节序 |
| 发送多字节消息头 | `htonl(len)` / `htons(type)` |
| 从消息头解析长度 | `ntohl(net_len)` |
| 从 accept 拿到客户端地址 | `ntohs(addr.sin_port)` |
| 逐字段序列化结构体 | 不要 `send(&struct)`，一个个字段转后发 |
| 消息队列里的数值 | 入队前序列化时转网络字节序，出队后反序列化转回 |

### 8.3 记住一句话

> **网络传输用大端，叫网络字节序。发前 htonx，收后 ntohx，结构体不要整个 send。**

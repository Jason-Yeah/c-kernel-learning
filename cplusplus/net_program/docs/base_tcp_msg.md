# TCP 粘包问题与消息队列设计——从底层原理到企业实践

> **目标读者**：已经写过简单的 socket 收发，发现数据经常粘在一起；听过"粘包"这个词但不清楚为什么粘、怎么解；想了解消息队列怎么在应用层实现。
>
> **阅读路线**：粘包是什么 → 底层为什么粘 → 三种解决方案 → 消息队列实现 → 企业级队列设计 → ASIO 中的完整示例

---

## 目录

1. [粘包是什么——一个直观的感受](#1-粘包是什么一个直观的感受)
2. [为什么 TCP 会粘包——底层原理](#2-为什么-tcp-会粘包底层原理)
3. [粘包的五种解决方案](#3-粘包的五种解决方案)
4. [消息队列（Message Queue）的设计](#4-消息队列message-queue的设计)
5. [企业级消息队列的实现](#5-企业级消息队列的实现)
6. [结合 ASIO 的完整示例——解决粘包的消息队列](#6-结合-asio-的完整示例解决粘包的消息队列)
7. [总结](#7-总结)

---

## 1. 粘包是什么——一个直观的感受

### 1.1 现象

假设你要发送两条消息：

```
消息 A: "Hello"
消息 B: "World"
```

你期望接收方收到的是两条独立的消息：

```
收到： "Hello"
收到： "World"
```

但实际收到的可能是：

```
收到： "HelloWorld"     ← 粘在一起了！
```

或者：

```
收到： "Hel"
收到： "loWorld"        ← 边界完全错乱
```

这种现象就叫做**粘包（sticky packet）**。

### 1.2 问题本质

**TCP 是流式协议（stream protocol），不是消息协议（message protocol）。**

```c
// 你发送时的逻辑：
send(fd, "Hello", 5, 0);
send(fd, "World", 5, 0);

// 内核眼里只有一堆字节流：
// "HelloWorld"  → 内核不关心你的消息边界
```

**UDP 没有粘包问题**——因为 UDP 是数据报协议（datagram protocol），每次 `recvfrom()` 收到的是一个完整的数据报，边界由内核维护。

**关键区别一句话**：

| | TCP | UDP |
|--|-----|-----|
| 传输方式 | 流（stream） | 数据报（datagram） |
| 消息边界 | **不保留** | 保留 |
| 粘包 | 有 | 无 |
| 可靠性 | 保证到达、有序 | 不保证 |

---

## 2. 为什么 TCP 会粘包——底层原理

### 2.1 从一次 send 调用到对端 recv

你的 `send(fd, "Hello", 5, 0)` 经历了什么：

```text
你的应用层: send("Hello")         send("World")
                │                    │
                ▼                    ▼
TCP 套接字发送缓冲区（内核）          │
                │                    │
                │  Nagle 算法：如果 "Hello" 很小（<MSS），
                │  等一会儿看有没有更多数据一起发
                │  → "Hello" 和 "World" 可能合并了！
                │
                ▼
TCP 段（Segment）→ 可能包含 "HelloWorld"
                │
                ▼
对端 TCP 接收缓冲区：收到 "HelloWorld"
                │
                ▼
对端 recv()：一次读出 "HelloWorld"
```

**粘包的核心原因**：**应用层的消息边界和 TCP 的传输边界不对齐。**

TCP 不看你的消息在哪里开始、哪里结束——它只看字节。所以多个小消息可能被合并（粘包），一个大消息也可能被拆分（半包）。

### 2.2 粘包的三个底层推手

#### 推手 1：Nagle 算法

Nagle 算法是 TCP 协议的一个优化。它的规则是：

```text
if (有数据未确认 && 数据量 < MSS) {
    等一会儿，攒够了再发；
} else {
    立即发送；
}
```

**目的**：减少小包数量。比如 1 字节的包 + 40 字节的 TCP/IP 头 = 41 字节，效率极低。Nagle 等攒够了再发。

**对粘包的影响**：多个小 send 合并在一个 TCP 段里发出 → 接收方一次性收到多个消息 → 粘包。

#### 推手 2：内核缓冲区合并

即使禁用了 Nagle 算法（`TCP_NODELAY`），内核仍然可能把多个 TCP 段合并：

```
应用层 send 的时序：
    send(A) → send(B)

内核缓冲区（发送端）：
    [AAAAA][BBBBB]  → 如果两个 send 间隔极短，
                       内核可能合并成一个 TCP 段发出
    [AAAAABBBBB]
```

#### 推手 3：接收端读取不及时

```
发送端连续发： A  B  C  D
                              ← 网络传输各段顺序到达
接收端缓冲区： [A][B][C][D]
                              ← 接收端 recv() 一次读了一大块
应用层读到：   "ABCD"          ← 粘包！
```

即使用户态代码没写错，如果接收方处理不够快，缓冲区里累积了多个消息，一次 `recv()` 就全读出来了。

### 2.3 总结：粘包是"必然"的吗？

```
TCP 一定会粘包吗？ → 不是必然，但大概率会。
```

**什么情况下不粘？**

- 每次 send 后间隔足够长（几十毫秒以上），Nagle 来不及合并
- 使用 TCP_NODELAY 且每次发送数据量 > MSS（最大报文段长度，通常 1460 字节）

**什么情况下必然粘？**

- 高频小消息（如游戏同步包、行情推送）
- 接收方处理慢，缓冲区堆积
- 网络延时导致多个包同时到达接收缓冲区

**严谨来说**：TCP 不保证消息边界，所以你**必须认为粘包会发生**，并在应用层解决。

---

## 3. 粘包的五种解决方案

所有解决方案的共同思路：**在应用层为 TCP 字节流加上"消息边界"**。

```
TCP 字节流:     Hel          loWorld          !!
                     ↑ 没有边界，谁是谁？

加上边界后:     [Hello]      [World]          [!!]
                     ↑ 有了边界，可以正确切分
```

### 3.1 方案一：固定长度（Fixed Length）

每个消息的长度都固定。接收方每次读 N 字节，读完一个就是一个完整消息。

```
协议格式：
[ 数据: N 字节 ]

例子（N=8）：
"Hello   "  ← 不足 8 字节用空格填充
"World   "
"!!      "
```

**代码实现——发送端：**

```cpp
const int MSG_SIZE = 8;

void send_fixed(tcp::socket& sock, const std::string& msg) {
    std::array<char, MSG_SIZE> buf{};
    // buf 已经全部初始化为 0
    memcpy(buf.data(), msg.data(), std::min(msg.size(), MSG_SIZE));

    boost::asio::write(sock, boost::asio::buffer(buf));
}
```

**代码实现——接收端：**

```cpp
void start_read(tcp::socket& sock) {
    auto buf = std::make_shared<std::array<char, MSG_SIZE>>();
    boost::asio::async_read(sock, boost::asio::buffer(*buf),
        [buf](boost::system::error_code ec, size_t n) {
            if (!ec) {
                // buf 里装着一个完整消息（固定 8 字节）
                process_message(buf->data(), n);
            }
        });
}
```

**优点**：实现极简单，不需要复杂的解析逻辑。

**缺点**：
- 浪费空间——短消息也要填充满 N 字节
- 不灵活——不能发超过 N 字节的消息
- 适合数据长度固定的协议（如某些物联网协议、旧版游戏协议）

### 3.2 方案二：特殊分隔符（Delimiter-Based）

消息之间用特殊字符分隔，如 `\n`（换行）、`\r\n`（HTTP 头结束）、`\0`（空字符）。

```
协议格式：
[ 数据 ] [ 分隔符 ] [ 数据 ] [ 分隔符 ]

例子（分隔符 = \n）：
"Hello\nWorld\n!!\n"
```

**代码实现——发送端：**

```cpp
void send_with_delimiter(tcp::socket& sock, const std::string& msg) {
    std::string buf = msg + "\n";  // 追加分隔符
    boost::asio::write(sock, boost::asio::buffer(buf));
}
```

**代码实现——接收端（使用 `async_read_until`）：**

```cpp
void start_read(boost::asio::ip::tcp::socket& sock,
                boost::asio::streambuf& buf) {
    boost::asio::async_read_until(sock, buf, '\n',  // ← 读到 \n 为止
        [&buf](boost::system::error_code ec, size_t n) {
            if (!ec) {
                // 从 streambuf 里取出一行
                std::istream is(&buf);
                std::string line;
                std::getline(is, line);  // 读到 \n 前为止

                // line 现在是一个完整消息（不含 \n）
                process_message(line);

                // 继续读下一行
                start_read(sock, buf);
            }
        });
}
```

**优点**：实现简单，HTTP、FTP、Redis RESP 协议都用这种方式。

**缺点**：
- 消息内容中不能包含分隔符（需要转义，如 HTTP 的 `Transfer-Encoding: chunked`）
- 性能不如长度前缀方案（需要逐个字节匹配分隔符）
- 恶意攻击者可以发超大消息没有分隔符耗尽内存

### 3.3 方案三：长度前缀（Length-Prefix / TLV）

每个消息的前面 N 个字节表示消息体长度。这是**最通用的方案**，几乎所有企业协议都用它。

```
协议格式：
[ 长度: 4字节 ] [ 数据: N字节 ]
  ↑  uint32_t     ↑ 实际数据

例子：
[0x00000005][Hello]
[0x00000005][World]
[0x00000002][!!]
```

**类型-长度-值（TLV，Type-Length-Value）变体**：

```
[ Type: 1字节 ][ Length: 4字节 ][ Value: N字节 ]
   ↑ 消息类型    ↑ 消息体长度    ↑ 消息体

例子：
[0x01][0x00000005][Hello]   ← 类型 1 的消息 "Hello"
[0x02][0x00000005][World]   ← 类型 2 的消息 "World"
```

TLV 的优点：接收方可以**先根据 type 分派到不同的处理函数**，再根据 length 读取对应数据。

**代码实现——发送端：**

```cpp
void send_with_length(tcp::socket& sock, const std::string& msg) {
    // 1. 构造缓冲区：4 字节长度 + 消息体
    uint32_t len = htonl(msg.size());  // 网络字节序
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&len, 4));
    buffers.push_back(boost::asio::buffer(msg));

    // 2. 一次（或分段）写入
    boost::asio::write(sock, buffers);
}
```

**为什么用 `async_read` 而不是 `async_read_some`？**

这是初学者最容易混淆的地方，也是博客里试图解释的核心。

```
async_read_some 的特性：
    对端发了数据 → 内核通知"可读" → async_read_some 的回调触发
    但：对端发 100 字节，你可能只读了 20 字节
    结果：回调里的 bytes_transferred 可能是 20，不是 100
    你得自己在回调里反复 async_read_some，直到凑够

async_read 的特性：
    你指定：我要读 4 字节
    async_read 内部反复调用 async_read_some
    直到真的凑满 4 字节了，才触发你的回调
    结果：回调里的 bytes_transferred 一定是 4
    你不需要自己循环
```

**所以 TLV 粘包的正确做法**：把"读 header"和"读 body"分别用 `async_read` 包起来，各读各的，互不干扰。

```
第一步：async_read(sock, buf(4))         → 读 4 字节 header（保证读完）
    拿到 body_len
第二步：async_read(sock, buf(body_len))  → 读 body_len 字节 body（保证读完）
    拿到完整消息
第三步：回到第一步（循环）
```

**代码实现——接收端（两步法，企业标准写法）：**

```cpp
// 一个 session 类里使用两步 async_read 的完整模式
// 内部自动处理粘包：async_read 保证读够了才回调

class MySession : public std::enable_shared_from_this<MySession> {
public:
    void start() {
        // 启动读循环：先从读 header 开始
        read_header();
    }

private:
    // ============================================
    // 第一步：读 HEAD_LEN 字节 header
    // async_read 保证读完 HEAD_LEN 字节才触发回调
    // 不需要检查 bytes_transferred < HEAD_LEN
    // ============================================
    void read_header() {
        auto self = shared_from_this();
        boost::asio::async_read(
            _sock,
            boost::asio::buffer(_header_buf),
            [self](boost::system::error_code ec, size_t n) {
                // n == HEAD_LEN，一定是（async_read 保证）
                if (ec) { self->handle_error(ec); return; }

                // 解析 body 长度
                uint32_t body_len;
                memcpy(&body_len, self->_header_buf.data(), HEAD_LEN);
                body_len = ntohl(body_len);   // ← 字节序转换

                if (body_len == 0 || body_len > MAX_BODY) {
                    self->close();  // 非法长度
                    return;
                }

                // 第二步：根据长度读 body
                self->read_body(body_len);
            });
    }

    // ============================================
    // 第二步：读 body_len 字节 body
    // async_read 保证读完 body_len 字节才触发回调
    // ============================================
    void read_body(uint32_t body_len) {
        auto self = shared_from_this();
        auto body = std::make_shared<std::vector<char>>(body_len);

        boost::asio::async_read(
            _sock,
            boost::asio::buffer(*body),
            [self, body](boost::system::error_code ec, size_t n) {
                // n == body_len，一定是
                if (ec) { self->handle_error(ec); return; }

                // body 里是一个完整消息，交给业务层处理
                self->on_message(body->data(), body->size());

                // 继续读下一个 header（形成循环）
                self->read_header();
            });
    }

    tcp::socket _sock;
    static constexpr size_t HEAD_LEN = 4;
    std::array<char, HEAD_LEN> _header_buf;
};
```

**关键点总结**：

| 博客里错的地方 | 企业正确做法 |
|--------------|------------|
| 检查 `bytes_transferred < HEAD_LENGTH` | 不需要——`async_read` 保证读够 |
| 缺了 `ntohl()` 字节序转换 | 必加，否则小端机拿到错误长度 |
| `std::bind` + `this` 裸指针 | lambda + `shared_from_this()` |
| 回调里 `sleep_for(2000ms)` | **永远不要**——阻塞 io_context 线程 |
| 用 `\0` 结尾当字符串处理 | 二进制数据不要当字符串 |

### 3.4 方案四：COBS 编码（适用于二进制分隔符协议）

COBS（Consistent Overhead Byte Stuffing，一致性开销字节填充）解决分隔符方案的核心痛点：**消息内容里不能出现分隔符**。

#### 3.4.1 问题场景

假设你用 `0x00` 作为消息分隔符：

```
send("Hello\x00World")  ← 消息 Hello 里包含了 0x00！
```

那接收方读到第一个 `0x00` 就以为消息结束了——数据被截断。传统做法是**转义**（像 C 语言的 `\0` 表示法），但转义需要扫描整个数据，且膨胀率不可控。

#### 3.4.2 COBS 的原理

COBS 的做法是：**把数据里所有的 `0x00` 替换成非零值，在末尾加一个 `0x00` 作为结束标记**。

```
原始数据：      [01] [02] [00] [03] [04]
                        ↑ 出现了分隔符！

COBS 编码后：   [03] [01] [02] [03] [03] [04] [00]
                 ↑                      ↑     ↑
              第一个块长度             没有0x00 结束符
              (跳过3字节到编码位置)

拆分理解：
  [03]          ← 从当前位置开始，后面 3 个字节不含 0x00
  [01][02]      ← 前 3 字节
  [03]          ← 再后面 3 个字节不含 0x00
  [03][04]      ← 后 3 字节
  [00]          ← 结束标记
```

**优点**：
- 消息体内可以包含任意字节（包括 `0x00`）
- 最大额外开销固定：每 254 字节只多 1 字节（≈0.4%）
- 编码/解码极快（只需异或和指针移动）

**缺点**：
- 比纯长度前缀方案多一次编解码开销

#### 3.4.3 COBS 在 C++ 中的实现

```cpp
// COBS 编码：将 src 编码到 dst，返回编码后长度
// 编码后以 0x00 结尾，内容中不含 0x00
size_t cobs_encode(const uint8_t* src, size_t len, uint8_t* dst) {
    size_t dst_idx = 0;
    size_t code_idx = 0;
    dst[code_idx] = 0;  // 占位，稍后填充
    dst_idx = 1;

    for (size_t i = 0; i < len; ++i) {
        if (src[i] == 0) {
            // 遇到 0x00：记录前一段的长度
            dst[code_idx] = static_cast<uint8_t>(dst_idx - code_idx);
            code_idx = dst_idx;
            dst[dst_idx++] = 0;
        } else {
            dst[dst_idx++] = src[i];
            // 每 254 个非零字节插入一个长度标记
            if (dst_idx - code_idx == 255) {
                dst[code_idx] = 255;
                code_idx = dst_idx;
                dst[dst_idx++] = 0;
            }
        }
    }

    // 最后一段的长度
    dst[code_idx] = static_cast<uint8_t>(dst_idx - code_idx);
    dst[dst_idx++] = 0;  // 结束标记
    return dst_idx;
}

// COBS 解码：逆操作
size_t cobs_decode(const uint8_t* src, size_t len, uint8_t* dst) {
    size_t src_idx = 0;
    size_t dst_idx = 0;

    while (src_idx < len - 1) {  // 最后一个是 0x00 结束符
        uint8_t code = src[src_idx++];
        for (uint8_t i = 1; i < code; ++i) {
            dst[dst_idx++] = src[src_idx++];
        }
        if (code != 255 && src_idx < len - 1) {
            dst[dst_idx++] = 0;  // 还原被替换的 0x00
        }
    }
    return dst_idx;
}

// 使用示例
void send_cobs(tcp::socket& sock, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> encoded(data.size() + data.size() / 254 + 2);
    size_t enc_len = cobs_encode(data.data(), data.size(), encoded.data());
    // 发送编码后的数据，接收方收到后解码，以 0x00 为边界切分
    boost::asio::write(sock, boost::asio::buffer(encoded.data(), enc_len));
}
```

**适用场景**：嵌入式系统、串口通信、某些工业协议（如 CANopen）。

### 3.5 方案五：复合操作（Composed Operation）模式

> ⚠️ **本方案涉及 ASIO 高级特性（`async_compose` + 模板元编程），是"可选项"不是"必选项"。** 
> 如果你现在只想用 `async_read` / `async_write` 搞定粘包，直接跳到下一节的应用层 ACK，或者回头用 3.3 的长度前缀 + 手动两步读（先读 header 再读 body）——那是你当前知识范围内最推荐的方案。
>
> 这里放出来是为了让你知道 ASIO 可以封装到这种程度，**以后需要了再回来看**。

这不是一个新的拆包方案，而是 ASIO 中**封装拆包逻辑的最佳代码组织方式**。你可以把"读一个完整消息"封装成一个可复用的 `async_read_message` 函数，内部自动处理粘包和半包。

```cpp
// ============================================================
// 一个完整的 Composed Operation：读一个 TLV 消息
// 对外使用就像 async_read(sock, buf, cb) 一样简单
// ============================================================
template<typename CompletionToken>
auto async_read_message(
    tcp::socket& sock,
    Message& out_msg,
    CompletionToken&& token)
{
    // async_compose 是 ASIO 提供的"组合操作"工具
    // 模板参数：
    //   CompletionToken           —— 用户传进来的回调类型（lambda / function pointer）
    //   void(boost::system::error_code) —— 最终回调的签名
    // 
    // 它返回一个"中间操作对象"，内部替你管理了：
    //   - 用户回调的存储和生命周期
    //   - 中间状态的传递
    //   - 最终回调的触发
  
    return boost::asio::async_compose<
        CompletionToken,                              // 用户回调的类型
        void(boost::system::error_code)               // 最终完成后调用的签名
    >(
        // ===== 第一个参数：操作实现的 lambda =====
        // 这个 lambda 会被多次调用——每次一个内部异步操作完成时
        //
        // [&, state] 的捕获策略：
        //   [&]  —— 引用捕获外部变量（sock、out_msg），
        //           保证它们在整个组合操作期间都活着
        //   state —— 以移动方式捕获一个 std::array<char,5> 作为状态
        //   mutable —— 因为 state 需要在 lambda 内部修改
        //
        // auto& self 是什么？
        //   self 的类型是内部实现类，由 async_compose 自动生成。
        //   你可以把它看作"这次组合操作的遥控器"——
        //   它有以下几个"按键"：
        //
        //   self.complete(ec)        —— 结束整个组合操作，回调用户的 handler
        //   self.complete({})        —— 同上，表示成功（无错误）
        //   self(ec, n)              —— 和 complete 等价，也能完成操作
        //   std::move(self)          —— 把遥控器传递给下一个 async_xxx 函数，
        //                               让 ASIO 知道"下一个操作是这次组合操作的一部分"
        //
        //   类型：auto& self 的实际类型是 async_compose 内部生成的
        //   「操作状态机」类型，开发者不需要知道具体名字——auto 就够了。
        //   核心要点：self 负责把多个异步操作串成一条链，
        //   并最终调用用户的 CompletionToken。
        //
        // 参数 ec 和 n：
        //   ec —— 上一步异步操作的错误码。第一次调用时 ec = {} (默认构造，即成功)
        //   n  —— 上一步异步操作传输的字节数。第一次调用时 n = 0
        //   每次 lambda 被调用时，ec 和 n 来自"上一步"的 async_read / async_write

        [&, state = std::array<char, 5>{}]
        (auto& self,                                             // ← "遥控器"
         boost::system::error_code ec = {},                      // ← 上一步的错误
         size_t n = 0) mutable {                                 // ← 上一步的字节数

            // 如果上一步出错了 → 终止整个组合操作，把错误传给用户
            if (ec) {
                self.complete(ec);  // 调用用户的回调函数(ec)
                return;
            }

            // ===== 三步状态机 =====
            // state[0] 用 0/1/2 表示当前进度：
            //   0 = 下一步应该读 header（5 字节）
            //   1 = header 读完了，下一步应该读 body
            //   2 = 全部读完了，调用用户的回调

            // ===== 第 1 步：读 5 字节 header =====
            // header 结构：[ type:1B | length:4B ]
            if (state[0] == 0) {
                boost::asio::async_read(
                    sock,
                    boost::asio::buffer(state.data(), 5),  // 读到 state 数组的前 5 字节
                    std::move(self)   // ← 把"遥控器"传进去。这次 async_read 完成后，
                                      //   会再次调用这个 lambda，ec 和 n 由它填充
                );
                state[0] = 1;  // 下次进来走第 2 步

            // ===== 第 2 步：根据 header 中的长度读 body =====
            } else if (state[0] == 1) {
                // 从已读的 header 中解析 body 长度
                uint32_t body_len;
                memcpy(&body_len, state.data() + 1, 4);  // state[1..4] 存着长度
                body_len = ntohl(body_len);              // 网络字节序 → 主机字节序
                out_msg.body.resize(body_len);           // 给 out_msg 分配空间

                boost::asio::async_read(
                    sock,
                    boost::asio::buffer(out_msg.body),    // 读到 out_msg.body 里
                    std::move(self)   // ← 再次传递"遥控器"。
                                      //   这次 async_read 完成后，还会再调用一次本 lambda
                );
                state[0] = 2;  // 下次进来走第 3 步

            // ===== 第 3 步：全部读完了，通知用户 =====
            } else {
                self.complete({});  // {} 表示 boost::system::error_code 默认构造 = 成功
                // 这行会触发用户的回调：callback(boost::system::error_code{})
                // 此时 out_msg 里已经装好了完整的消息体
            }
        },
        token,    // 用户的回调（CompletionToken）
        sock      // 关联的 socket（用于 executor 转发）
    );
}

// 使用——一行代码读一个完整消息，粘包自动处理
Message msg;
async_read_message(sock, msg, [](boost::system::error_code ec) {
    if (!ec) {
        // msg 已经是一个完整消息了
        process(msg);
    }
});
```

**优点**：调用方只需一行代码，拆包逻辑被封装在组合操作内部，与业务完全解耦。

**但你现在不需要这个**——回到方案三（长度前缀）的两步法，用 `async_read` 先读 header 再读 body，已经能解决 95% 的粘包场景。你当前已经会了：

```cpp
// 第一步：读 4 字节 header
asio::async_read(sock, asio::buffer(header_buf), [](ec, n) {
    // 第二步：读 body  
    asio::async_read(sock, asio::buffer(body_buf), [](ec, n) {
        // 消息完整了
    });
});
```

`async_compose` 只是把上面这段封装成函数，逻辑完全一样。**不封装也能用，不耽误干活。**

### 3.6 ⚠️ 澄清：ACK + 序列号不解决粘包

这是一个常见误解。先区分两个不同的问题：

```
粘包：send("Hello") + send("World") → recv("HelloWorld")   → 数据都在，但不知道边界
丢包：send("Hello") → 网络丢包 → recv 没收到                → 数据没了
```

**ACK + 序列号解决丢包/重复/乱序，不解决粘包。**

粘包的解法是**长度前缀/分隔符**（方案一/二/三/四）。SEQ 和 ACK 只是搭了长度前缀的便车放在包头里——真正用来切分消息的永远是长度字段，不是 SEQ。

### 3.7 五种方案总对比

| 维度 | 固定长度 | 分隔符 | 长度前缀 | **COBS** | **复合操作** |
|------|---------|--------|---------|---------|------------|
| 变长支持 | ❌ | ✅ | ✅ | ✅ | ✅ |
| 内容含分隔符 | ✅ | ❌ → ⚠️ 需转义 | ✅ | ✅ | ✅ |
| 空间开销 | 高（填充） | 低（1B） | 低（4B） | 0.4% | 同长度前缀 |
| 实现复杂度 | 极简 | 简单 | 中等 | 中等 | 较高 |
| 抗攻击（大包） | ✅ | ❌ | ✅ | ✅ | ✅ |
| 企业使用 | 极少 | HTTP/Redis | **主流** | 嵌入式 | ASIO 高级用法 |
| 协议示例 | 旧版游戏 | HTTP/1.1, Redis | gRPC, MQTT | CANopen | 自定义 |

### 3.8 你该选哪种？

这是一个决策树：

```
你的消息内容是否可能包含分隔符？
├─ 是 → 内容长度是否固定？
│   ├─ 是 → 固定长度（最简单）
│   └─ 否 → 长度前缀 / TLV（最通用）
│
└─ 否 → 你更关心什么？
    ├─ 实现简单 → 分隔符方案
    ├─ 空间效率 → 长度前缀
    └─ 嵌入式/串口 → COBS

最终建议：
  95% 的场景 → 长度前缀 / TLV
  3% 的场景  → 分隔符（文本协议）
  2% 的场景  → 固定长度 / COBS / 其他
```

> **一句话原则**：没有特殊理由就用长度前缀（TLV），这是经过 gRPC、MQTT、Kafka 等无数企业项目验证过的选择。

| 维度 | 固定长度 | 分隔符 | 长度前缀 |
|------|---------|--------|---------|
| 实现复杂度 | 极简 | 简单 | 中等 |
| 空间效率 | 低（浪费填充） | 中（分隔符占 1 字节） | **高**（仅 4 字节开销） |
| 支持变长 | ❌ | ✅ | ✅ |
| 安全性 | 高（固定大小） | 低（可能耗尽内存） | **高**（可检查长度上限） |
| 性能 | 高 | 中（逐字节匹配） | **高** |
| 企业使用 | 少 | HTTP、Redis | **最多**（gRPC、MQTT、自定义协议） |

**企业推荐**：长度前缀方案（TLV）。灵活性高、安全可控、性能好。

---

## 4. 消息队列（Message Queue）的设计

解决了粘包问题后，下一个问题是：**收到消息后怎么传递给业务逻辑？**

如果你的 echo server 是收到什么就立即回复什么，那不需要消息队列。但真实应用往往是：

```
收包线程 → [消息队列] → 处理线程
                ↑
            解耦生产者和消费者
```

### 4.1 为什么需要消息队列

```
没有消息队列（直连）：
    网络线程 → 直接调用处理函数
              ↓
          处理函数很慢（如查询数据库）
              ↓
          网络线程被阻塞 → 其他连接收不到新数据

有消息队列（解耦）：
    网络线程 → push(msg) → [消息队列] → pop() → 处理线程
      (快)                                 (慢，但没关系)
          网络线程从不阻塞，所有连接都能及时收到新数据
```

### 4.2 一个简单的消息队列实现

```cpp
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class MessageQueue {
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(item));
        _cond.notify_one();  // 唤醒一个等待的消费者（不缺的唤醒，根据当前CPU调度情况）
    }

    T pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        // wait中条件不满足时，释放锁线程挂起，进入等待队列直到被notify唤醒，
        // 唤醒后重新获取锁
        // 再检查条件是否满足
        _cond.wait(lock, [this] { return !_queue.empty(); });
        // 当队列非空时返回
        T item = std::move(_queue.front());
        _queue.pop();
        return item;
    }

    bool try_pop(T& item) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) return false;
        item = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _cond;
};
```

**使用：**

```cpp
MessageQueue<std::string> mq;

// 网络线程（生产者）
void on_message_received(const std::string& msg) {
    mq.push(msg);  // 快速入队，不阻塞网络线程
}

// 处理线程（消费者）
void worker_thread() {
    while (true) {
        std::string msg = mq.pop();  // 队列空时阻塞等待
        process(msg);                // 慢处理，不影响网络线程
    }
}
```

### 4.3 这个实现的问题

1. **锁竞争**——每次 push/pop 都加锁，高并发下锁是瓶颈
2. **堆分配**——`std::queue` 内部用链表或 deque，每次 push 分配内存
3. **`notify_one` 的惊群**——多个消费者同时被唤醒（多生产-多消费场景）
4. **无界队列**——消费者太慢时队列无限增长，耗尽内存

---

## 5. 企业级消息队列的实现

企业级消息队列要解决上面那几个问题。以下是生产环境常用的技术。

### 5.1 有界队列（Bounded Queue）——防止内存耗尽

```cpp
template<typename T>
class BoundedQueue {
public:
    BoundedQueue(size_t max_size) : _max_size(max_size) {}

    bool push(T item) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.size() >= _max_size) {
            return false;  // 队列满了，丢了（或拒绝）
        }
        _queue.push(std::move(item));
        _cond.notify_one();
        return true;
    }

    // ...

private:
    std::queue<T> _queue;
    size_t _max_size;
    std::mutex _mutex;
    std::condition_variable _cond;
};
```

**满队列的处理策略**：

| 策略 | 做法 | 适用场景 |
|------|------|---------|
| **阻塞** | push 时等待队列有空位 | 生产者消费者速度匹配 |
| **丢弃** | push 时满了直接扔 | 实时行情（旧数据比新数据重要） |
| **覆盖** | 满了时覆盖最旧的一条 | 日志（保留最新） |
| **反压** | 通知上游"慢一点" | 流量控制，最优雅的方式 |

### 5.2 环形缓冲区（Ring Buffer / Circular Buffer）——避免堆分配

`std::queue` 每次 push 都分配内存。环形缓冲区预分配好内存，push 和 pop 只在数组里移动指针，**零分配**。

```
预分配数组： [  |  |  |  |  |  |  |  ]
               ↑               ↑
             head (读位置)    tail (写位置)

push A:      [ A |  |  |  |  |  |  |  ]
               ↑       ↑
             head    tail

push B:      [ A | B |  |  |  |  |  |  ]
               ↑           ↑
             head        tail

pop → A:     [ A | B |  |  |  |  |  |  ]
                   ↑       ↑
                 head    tail

满时：        [ X | X | X | X | X | X | X | X ]
                                       ↑   ↑
                                     tail head
                                      (tail 追上 head)
```

```cpp
template<typename T>
class RingBuffer {
public:
    RingBuffer(size_t capacity)
        : _buffer(capacity)
        , _head(0), _tail(0), _full(false) {}

    bool push(const T& item) {
        if (_full) return false;

        _buffer[_tail] = item;
        _tail = (_tail + 1) % _buffer.size();

        if (_tail == _head) _full = true;
        return true;
    }

    bool pop(T& item) {
        if (empty()) return false;

        item = _buffer[_head];
        _head = (_head + 1) % _buffer.size();
        _full = false;
        return true;
    }

    bool empty() const {
        return !_full && _head == _tail;
    }

private:
    std::vector<T> _buffer;
    size_t _head;    // 读指针
    size_t _tail;    // 写指针
    bool _full;      // 满标志
};
```

### 5.3 多生产者多消费者（MPMC）——无锁队列

当有多个网络线程（多生产者）和多个处理线程（多消费者）时，锁竞争成为瓶颈。**无锁队列（lock-free queue）** 用 CAS（Compare-And-Swap）原子操作代替锁。

```cpp
// 这是一个简化的单生产者单消费者（SPSC）无锁队列
// 完整的多生产者多消费者（MPMC）实现要复杂得多

template<typename T>
class LockFreeSPSCQueue {
public:
    LockFreeSPSCQueue(size_t capacity)
        : _buffer(capacity), _head(0), _tail(0) {}

    bool push(const T& item) {
        size_t tail = _tail.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % _buffer.size();

        if (next == _head.load(std::memory_order_acquire))
            return false;  // 满了

        _buffer[tail] = item;
        _tail.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t head = _head.load(std::memory_order_relaxed);

        if (head == _tail.load(std::memory_order_acquire))
            return false;  // 空的

        item = _buffer[head];
        _head.store((head + 1) % _buffer.size(),
                     std::memory_order_release);
        return true;
    }

private:
    std::vector<T> _buffer;
    std::atomic<size_t> _head;
    std::atomic<size_t> _tail;
};
```

#### 5.3.1 std::atomic 基础——无锁队列的基石

上面的 `LockFreeSPSCQueue` 里用到了一个你看不懂的关键东西：`std::atomic<size_t>`。它就是无锁队列能"不加锁"的秘密。

##### 5.3.1.1 为什么需要 atomic？

多线程读写同一个变量时，会有**数据竞争（data race）**：

```cpp
// 两个线程同时执行，会发生什么？
// 线程 1：_tail = _tail + 1;
// 线程 2：_tail = _tail + 1;

// 你以为的结果：_tail 增加了 2
// 实际可能：_tail 只增加了 1！

// 原因：_tail = _tail + 1 在 CPU 层面是三步：
//   ① 从内存读 _tail 到寄存器       ← 线程1 和 线程2 可能同时读到同一个值
//   ② 寄存器里 +1
//   ③ 写回内存                     ← 两个线程都写了同一个值，其中一个覆盖了另一个
```

用 `std::mutex` 加锁能解决这个问题，但锁有开销。`std::atomic` 提供了另一种方式——**原子操作**。

##### 5.3.1.2 atomic 保证什么

`std::atomic<T>` 保证对 T 的读写是**不可分割的（atomic）**：

```cpp
std::atomic<int> counter{0};

// 线程 1
counter.fetch_add(1);   // 相当于 counter += 1，但保证不会被其他线程打断

// 线程 2
counter.fetch_add(1);   // 无论两个线程怎么交错，结果一定是 2

// 等价于：
//   lock add [counter], 1    ← CPU 层面用一个指令完成读+改+写
//   而不是：
//   mov eax, [counter]        ← 读
//   add eax, 1                ← 改
//   mov [counter], eax        ← 写（三步之间可能被中断）
```

##### 5.3.1.3 atomic 的三种核心操作

| 操作 | 函数 | 效果 | 类比 mutex 版本 |
|------|------|------|----------------|
| **读** | `_tail.load()` | 原子地读取当前值 | `lock(); auto v = _tail; unlock();` |
| **写** | `_tail.store(v)` | 原子地写入新值 | `lock(); _tail = v; unlock();` |
| **CAS** | `_head.compare_exchange_weak(old, new)` | 如果当前值 == old，就替换成 new | 无直接类比 |

**CAS（Compare-And-Swap）是无锁队列的灵魂**：

```cpp
// CAS 的逻辑（伪代码）：
bool atomic::compare_exchange_weak(T& expected, T desired) {
    if (当前值 == expected) {      // 检查：没人改过？
        当前值 = desired;           // 是 → 改成新值
        return true;
    } else {
        expected = 当前值;          // 否 → 告诉我现在是多少
        return false;
    }
    // 整个过程是一条 CPU 指令：lock cmpxchg
}

// CAS 的典型用法——抢着做一件事，只有一个线程能成功：
std::atomic<int> flag{0};

// 三个线程同时执行这段：
int expected = 0;
if (flag.compare_exchange_weak(expected, 1)) {
    // 只有第一个执行到这里（flag 从 0→1），其他线程 flag≠0 失败
    do_once();  // 只会执行一次
}
```

##### 5.3.1.4 memory_order——最让人头疼的部分

`_tail.load(std::memory_order_relaxed)` 里的 `memory_order_relaxed` 是什么？

它告诉 CPU 和编译器：**这个原子操作的"可见性"需要多严格**。

| memory_order | 意思 | 代价 | 什么时候用 |
|-------------|------|:---:|-----------|
| `relaxed` | 只保证原子性，不保证顺序。别的线程可能看到乱序的结果 | 零额外开销 | 计数器、统计量 |
| `acquire` | 这个 load 之后的读写，一定排在这个 load **之后**（不会越过它跑到前面去） | 小 | pop 时读 tail——确保看到最新的数据 |
| `release` | 这个 store 之前的读写，一定排在这个 store **之前**（不会拖到后面） | 小 | push 时写 tail——确保数据先写入再更新指针 |
| `acq_rel` | acquire + release | 中等 | read-modify-write 操作 |
| `seq_cst` | 全局严格顺序（默认值） | 最大 | 不太确定时用这个，保险但慢 |

**具体到无锁队列中**：

```cpp
// push 时（生产者）：
_buffer[tail] = item;                    // ① 先把数据写进 buffer
_tail.store(next, memory_order_release); // ② 再更新 tail 指针
// 为什么需要 release？
//   保证 ① 一定在 ② 之前完成。
//   消费者看到 tail 更新了，就能安全地读 _buffer[tail]
//   如果没有 release，CPU 可能把 ② 重排到 ① 前面
//   → 消费者看到 tail 更新了，但数据还没写进去 → 读到垃圾！

// pop 时（消费者）：
size_t head = _head.load(memory_order_relaxed);
// 为什么用 relaxed？
//   只用 head 来判断队列空不空，不需要同步其他数据

if (head == _tail.load(memory_order_acquire))
// 为什么需要 acquire？
//   保证看到的是生产者 release 之前的所有写入
//   如果没有 acquire，可能看到旧的 tail 值
//   → 认为队列为空，但其实生产者刚 push 了数据

item = _buffer[head];                    // ③ 安全了：release-acquire 保证了可见性
_head.store(next, memory_order_release); // ④ 更新 head
```

##### 5.3.1.5 shared_ptr 和 atomic

`std::shared_ptr` 的控制块（引用计数）内部用的就是 `std::atomic`——每次拷贝 `shared_ptr` 时，引用计数的增减都是原子的：

```cpp
auto sp1 = std::make_shared<int>(42);
auto sp2 = sp1;  // 引用计数从 1→2，这个 +1 是原子的
// 多个线程同时拷贝和销毁 shared_ptr，引用计数不会出错
// 这就是 shared_ptr "线程安全"的部分——控制块是原子操作
```

##### 5.3.1.6 一句话总结 atomic

```
std::atomic<T>  =  mutex 的轻量替代，用于单个变量的读写
    CAS (compare_exchange) = 无锁队列的灵魂，三个步骤合成一条 CPU 指令
    memory_order          = 告诉 CPU 可见性的严格程度，release/acquire 是出镜率最高的
```

**无锁 vs 有锁**：

| | 有锁队列（mutex） | 无锁队列（CAS） |
|--|-----------------|----------------|
| 低竞争（<4 线程） | ✅ 好 | ✅ 差不多 |
| 高竞争（>8 线程） | ❌ 锁争用严重 | ✅ 好 |
| 实时性 | ❌ 可能被优先级反转 | ✅ 可预测 |
| 实现复杂度 | 简单 | 极难（ABA 问题、内存序） |

**推荐**：除非你确定锁是瓶颈，否则先用 `mutex` 版本。无锁队列的实现极其容易出 bug，企业通常使用经过验证的库（如 `boost::lockfree::queue`、`folly::MPMCQueue`）。

### 5.4 内存池（Memory Pool）——避免频繁 new/delete

消息在入队出队过程中不断创建和销毁，如果每次都用 `new` / `delete`，不仅慢还会产生内存碎片。

**内存池**预分配一大块内存，切成固定大小的块：

```cpp
template<typename T>
class MemoryPool {
public:
    MemoryPool(size_t block_count) {
        for (size_t i = 0; i < block_count; ++i) {
            _free_list.push(new T());
        }
    }

    T* allocate() {
        if (_free_list.empty()) {
            // 没有空闲块，分配新的（或者等待）
            return new T();
        }
        T* ptr = _free_list.front();
        _free_list.pop();
        return ptr;
    }

    void deallocate(T* ptr) {
        _free_list.push(ptr);  // 归还给池，不是真的释放
    }

private:
    std::queue<T*> _free_list;
    // 注意：实际生产环境需要线程安全
};
```

**为什么企业用内存池？**

```
每次 new/delete（常规）：
    new → 系统调用 → 堆管理 → 可能慢几微秒
    delete → 系统调用 → 可能产生碎片

内存池（预分配）：
    allocate → 取一个现成的块 → O(1)
    deallocate → 放回去 → O(1)
    没有系统调用，没有碎片
```

### 5.5 企业消息队列架构全景

一个典型的企业级消息队列（如自研的游戏服务器消息队列、交易系统消息队列）通常是多层架构：

```
[网络线程池]                  [消息队列]              [工作线程池]
     │                           │                        │
     ├── 线程 1 recv ────┐       │                        │
     ├── 线程 2 recv ────┤       │     ┌─── 线程 A 处理 ──┤
     ├── 线程 3 recv ────┤→→→ [MPMC] →→┼─── 线程 B 处理 ──┤
     └── 线程 4 recv ────┘   有界  │     └─── 线程 C 处理 ──┤
                              无锁  │                        │
                              环形  │                        │
                                   │                        │
                              这样：网络线程从不阻塞
                                   工作线程不会因新连接而干扰
                                   队列满时会反压网络层
```

**分层职责**：

| 层 | 职责 | 技术 |
|----|------|------|
| 网络层 | 收数据、解包、拼包 | ASIO async_read + 长度前缀 |
| 消息队列 | 解耦网络层与业务层 | 有界环形缓冲区 / MPMC 无锁队列 |
| 业务层 | 处理消息（慢操作） | 线程池 |

---

## 6. 结合 ASIO 的完整示例——解决粘包的消息队列

> 本节只用到了你已经会的：`async_read` / `async_write` + `shared_from_this()` + `std::mutex` / `std::condition_variable`（标准 C++ 线程同步）。
> 
> `shared_from_this()` 的原理在 `base_asio_async.md` 第 4.4 节讲过。`BoundedMsgQueue` 里的 `mutex` 和 `condition_variable` 是标准库内容，不是 ASIO 的——你只需要知道"push 时加锁、pop 时等通知"就够了，不用深究。

### 6.1 完整协议定义

使用 **TLV 格式**：1 字节类型 + 4 字节长度 + N 字节数据。

```
[ Type: 1B ][ Length: 4B ][ Payload: N B ]
```

### 6.2 消息结构

```cpp
// 消息头（固定 5 字节）
struct MessageHeader {
    uint8_t  type;      // 消息类型
    uint32_t length;    // 消息体长度（网络字节序）
};

// 完整消息
struct Message {
    uint8_t              type;
    std::vector<char>    body;   // 消息体数据
};
```

### 6.3 有界消息队列

```cpp
class BoundedMsgQueue {
public:
    explicit BoundedMsgQueue(size_t max_size)
        : _max_size(max_size) {}

    bool push(Message msg) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.size() >= _max_size) {
            return false;  // 队列满，反压
        }
        _queue.push(std::move(msg));
        _cond.notify_one();
        return true;
    }

    Message pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this] { return !_queue.empty(); });
        Message msg = std::move(_queue.front());
        _queue.pop();
        return msg;
    }

private:
    std::queue<Message> _queue;
    size_t _max_size;
    std::mutex _mutex;
    std::condition_variable _cond;
};
```

### 6.4 Session：解包 + 入队

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, BoundedMsgQueue& msg_queue)
        : _sock(std::move(socket))
        , _msg_queue(msg_queue)
    {}

    void start() {
        read_header();
    }

private:
    void read_header() {
        auto self = shared_from_this();
        boost::asio::async_read(
            _sock, boost::asio::buffer(_header_buf),
            [self](boost::system::error_code ec, size_t) {
                if (ec) { self->handle_error(ec); return; }

                // 解析 header: [ type:1B | length:4B ]
                auto& hdr = self->_header_buf;
                uint8_t type   = hdr[0];
                uint32_t len;
                memcpy(&len, hdr.data() + 1, 4);
                // network to host long<arpa/inet.h>
                len = ntohl(len);

                if (len > MAX_BODY_SIZE) {
                    // 非法长度，断开连接
                    return;
                }

                self->read_body(type, len);
            });
    }

    void read_body(uint8_t type, uint32_t len) {
        auto self = shared_from_this();
        auto body = std::make_shared<std::vector<char>>(len);

        boost::asio::async_read(
            _sock, boost::asio::buffer(*body),
            [self, body, type](boost::system::error_code ec, size_t) {
                if (ec) { self->handle_error(ec); return; }

                // 组装完整消息
                Message msg;
                msg.type = type;
                msg.body = std::move(*body);

                // 入队（交给业务线程处理）
                if (!self->_msg_queue.push(std::move(msg))) {
                    // 队列满了，处理反压（如丢弃或通知客户端）
                }

                // 继续读下一个消息
                self->read_header();
            });
    }

    void handle_error(boost::system::error_code ec) {
        if (ec == boost::asio::error::eof) {
            // 客户端断开
        } else {
            // 其他错误
        }
    }

    tcp::socket _sock;
    std::array<char, 5> _header_buf;  // type(1) + length(4)
    BoundedMsgQueue& _msg_queue;
};
```

### 6.5 业务处理线程

```cpp
void worker_thread(BoundedMsgQueue& msg_queue) {
    while (true) {
        Message msg = msg_queue.pop();

        switch (msg.type) {
            case 0x01:  // 登录请求
                handle_login(msg.body);
                break;
            case 0x02:  // 聊天消息
                handle_chat(msg.body);
                break;
            case 0x03:  // 心跳
                // 心跳只需更新最后活动时间
                break;
            default:
                // 未知消息类型，记录日志
                break;
        }
    }
}
```

### 6.6 完整服务端架构

```cpp
int main() {
    // 1. 创建消息队列（有界，最多 10000 条）
    BoundedMsgQueue msg_queue(10000);

    // 2. 创建工作线程
    // std::ref<functional> 可拷贝引用类型，对不可拷贝类型，让这里科研传递引用不拷贝
    std::thread worker(worker_thread, std::ref(msg_queue));

    // 3. 启动网络服务
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 9190));

    std::function<void()> do_accept;
    do_accept = [&] {
        auto sock = std::make_shared<tcp::socket>(io);
        acceptor.async_accept(*sock, [&, sock](boost::system::error_code ec) {
            if (!ec) {
                auto session = std::make_shared<Session>(
                    std::move(*sock), msg_queue);
                session->start();
            }
            do_accept();
        });
    };
    do_accept();

    io.run();
    worker.join();
    return 0;
}
```

### 6.7 架构图总结

```
[客户端]                  [服务端]
                          
send("Hello")            boost::asio::async_read(sock, header_buf)
    │                              │ 读 5 字节 header
    │                              │
    │                    read_header():    [ type:1 | len:4 ]
    │                              │
    │                    read_body(len):   [ data: len bytes ]
    │                              │
    │                    拼成完整 Message
    │                              │
    │                    BoundedMsgQueue::push(msg)
    │                              │
    │                    ┌──────────┴──────────┐
    │                    │   消息队列（有界）    │
    │                    └──────────┬──────────┘
    │                              │
    │                    worker_thread::pop()
    │                              │
    │                    switch(msg.type) → 业务处理
```

---

## 7. 总结

### 7.1 粘包的核心

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 粘包 | TCP 是流协议，不保留消息边界 | 在应用层加边界 |
| 三种方案 | 固定长度 / 分隔符 / 长度前缀 | **企业推荐长度前缀（TLV）** |

### 7.2 消息队列的核心

| 需求 | 方案 |
|------|------|
| 解耦网络层与业务层 | 生产者-消费者消息队列 |
| 防止内存耗尽 | 有界队列（Bounded Queue） |
| 减少堆分配 | 环形缓冲区（Ring Buffer） |
| 避免锁竞争 | 无锁队列（Lock-Free MPMC） |
| 减少 new/delete 开销 | 内存池（Memory Pool） |

### 7.3 企业实践铁律

```
1. 总是加消息边界 —— TCP 是流，不是消息
2. 用长度前缀方案 —— 灵活、安全、高性能
3. 队列必须是有界的 —— 没有 "无限" 的内存
4. 网络线程不做慢操作 —— 入队列就返回
5. 业务线程不碰网络 —— 出队列再处理
6. 队列满了要反压 —— 而不是无限增长
```

---

> **下一步**：你可以把 `base_asio_async.md` 中的 session 改造为使用 TLV 协议的版本，配合这里的 BoundedMsgQueue，写出一个完整的、能解决粘包问题的生产级网络程序。
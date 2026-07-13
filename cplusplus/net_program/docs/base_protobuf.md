# Protobuf 入门——在网络编程中用来干什么

> **目标读者**：写过 socket、知道粘包和字节序，但每次手动序列化/反序列化代码又臭又长。想知道 protobuf 是什么、跟之前讲的内容有什么关联、怎么装怎么用、怎么配合 CMake 构建网络程序。

---

## 目录

1. [Protobuf 是什么](#1-protobuf-是什么)
2. [为什么需要 protobuf——手动序列化的痛点](#2-为什么需要-protobuf手动序列化的痛点)
3. [Protobuf 与之前内容的关系](#3-protobuf-与之前内容的关系)
4. [WSL/Ubuntu 下安装](#4-wslubuntu-下安装)
5. [快速上手：.proto 文件 → C++ 代码](#5-快速上手proto-文件--c-代码)
6. [Protobuf 的核心 API](#6-protobuf-的核心-api)
7. [CMake 入门与 protobuf 集成](#7-cmake-入门与-protobuf-集成)
8. [Protobuf + TLV 粘包方案——网络编程实战](#8-protobuf--tlv-粘包方案网络编程实战)
9. [总结](#9-总结)

---

## 1. Protobuf 是什么

**Protobuf（Protocol Buffers）** 是 Google 开源的**序列化框架**。它的核心功能只有一条：

> **把你代码里的结构体（struct）转成一串字节，或者反过来。**

```cpp
// 你代码里的数据（C++ struct）
struct Person {
    int id;
    string name;
    string email;
};

// 你把它变成字节流（序列化）
// → [0x08][0x2A][0x12][0x05][0x48][0x65][0x6C][0x6C][0x6F]...
//    ↑ id=42 ↑ name="Hello" 的编码

// 通过网络发出去，另一端收到字节流
// → 反序列化回 Person{id=42, name="Hello"}
```

跟手动序列化（你手写 `htonl` + `memcpy`）的区别：

| | 手动序列化 | Protobuf |
|--|----------|----------|
| 写 .proto 文件 | ❌ 不需要 | ✅ |
| 代码量 | `memcpy`、`htonl` 写一堆 | 自动生成序列化代码 |
| 跨语言 | 自己处理 | C++/Java/Python/Go 全支持 |
| 字节序 | 自己调 `htonl` | 自动编码（varint 不存在字节序问题） |
| 版本兼容 | 加字段必须改协议 | ✅ 后向/前向兼容 |
| 性能 | 最快（但写起来累） | 比手动慢 5~10%，但开发效率高百倍 |

---

## 2. 为什么需要 protobuf——手动序列化的痛点

回顾 `base_byteorder.md` 中的 TLV 协议：

```cpp
// 手动序列化：发一个 Person 结构
struct Person {
    uint32_t id;       // 4 字节
    char name[32];     // 32 字节
    uint32_t age;      // 4 字节
};

void send_person(tcp::socket& sock, const Person& p) {
    uint32_t net_id  = htonl(p.id);       // 转字节序
    uint32_t net_age = htonl(p.age);       // 转字节序

    // 自己拼 buffer 发出去
    std::array<asio::const_buffer, 3> bufs = {
        asio::buffer(&net_id, 4),
        asio::buffer(p.name, 32),
        asio::buffer(&net_age, 4)
    };
    asio::write(sock, bufs);
}
```

**痛点来了：**

1. **每个字段都要手动 `htonl`**——Person 有 3 个字段还勉强，几十个字段就灾难了
2. **不能发变长数据**——`name[32]` 固定 32 字节，短名浪费空间，长名装不下
3. **加字段要通知所有人**——服务器加了 `email` 字段，旧客户端不认识，还得兼容
4. **跨语言时再写一遍**——换 Java 客户端，`htonl` 的代码全部重写

**protobuf 把上面这些都解决了。**

---

## 3. Protobuf 与之前内容的关系

之前讲的内容和 protobuf 是**互补关系**，不是替代关系：

```
基础传输            消息边界            序列化内容
┌────────┐         ┌────────┐         ┌──────────┐
│ socket │ ───→    │ 粘包解决 │ ───→   │ protobuf │
│ ASIO   │         │ TLV    │         │           │
│ TCP    │         │ 长度前缀 │         │ 把结构体  │
└────────┘         └────────┘         │ 变成字节  │
                                      └──────────┘
```

| 之前讲的内容 | 和 protobuf 的关系 |
|-------------|-------------------|
| **粘包** | protobuf 不解决粘包。protobuf 序列化后的字节流 = TLV 中的 **V（Value）**。粘包仍然靠**长度前缀**切分 |
| **字节序** | protobuf 内部用 **varint** 编码，varint 是小端序，但**不依赖平台**——无论大端小端机器，编码结果相同。你不需要调 `htonl` |
| **消息队列** | 队列里存的就是 protobuf 序列化后的 `std::string` / `std::vector<char>` |
| **echo server** | 第 8 节会把之前写的 echo server 改成 protobuf 版本 |

**一句话**：protobuf = 你结构体的"自动序列化器"，它不涉及传输也不涉及粘包，只管"内存 ↔ 字节流"的转换。

---

## 4. WSL/Ubuntu 下安装

### 4.1 安装 protobuf 编译器 + 库

```bash
# Ubuntu 24.04
sudo apt update
sudo apt install -y protobuf-compiler libprotobuf-dev
```

验证安装：

```bash
protoc --version
# 输出：libprotoc 25.1（版本号可能不同）

# 编译器：protoc          → 把 .proto 文件生成 C++ 代码
# 运行时库：libprotobuf   → 序列化/反序列化需要的动态库
```

### 4.2 安装 CMake

```bash
sudo apt install -y cmake
cmake --version
# 输出：cmake version 3.28.3 或类似
```

---

## 5. 快速上手：.proto 文件 → C++ 代码

### 5.1 写一个 .proto 文件

```protobuf
// person.proto
syntax = "proto3";                    // 使用 proto3 语法

package demo;                         // 命名空间

message Person {
    int32  id     = 1;                // 字段名 = id, 类型 = int32, 编号 = 1
    string name  = 2;                 // 类型 = string, 编号 = 2
    int32  age   = 3;                 // 编号 = 3
    string email = 4;                 // 编号 = 4
}
```

**关键概念**：

- **`message`** = C++ 的 `struct` / `class`
- **字段类型**：`int32`、`string`、`bool`、`float`、`repeated`（数组）等
- **字段编号（=1, =2, ...）**：最重要——这是 protobuf 识别字段的凭据，不是字段名。编号一旦确定就**不要改**，否则旧数据反序列化会错

### 5.2 编译 .proto 文件

```bash
protoc --cpp_out=. person.proto
```

生成两个文件：

```
person.proto
person.pb.h     ← C++ 头文件（包含 Person 类的声明）
person.pb.cc    ← C++ 源文件（包含序列化代码的实现）
```

### 5.3 在 C++ 中使用生成的类

```cpp
#include "person.pb.h"
#include <iostream>

int main() {
    // 创建一个 Person 对象并赋值
    demo::Person p;
    p.set_id(42);
    p.set_name("Alice");
    p.set_age(30);
    p.set_email("alice@example.com");

    // 序列化：Person → 字节流
    std::string serialized;
    p.SerializeToString(&serialized);   // 序列化到 string
    // serialized 就是一串字节，可以直接 send / 入队列

    std::cout << "Serialized size: " << serialized.size() << " bytes\n";

    // 反序列化：字节流 → Person
    demo::Person p2;
    p2.ParseFromString(serialized);     // 从 string 反序列化

    std::cout << "Name: " << p2.name() << "\n";
    std::cout << "Age:  " << p2.age() << "\n";

    return 0;
}
```

### 5.4 编译这个例子

```bash
g++ -std=c++17 person.pb.cc main.cpp -lprotobuf -o main
./main
# 输出：
# Serialized size: 26 bytes
# Name: Alice
# Age:  30
```

**注意**：需要同时编译 `person.pb.cc` 和你的 `main.cpp`，链接 `-lprotobuf`。

---

## 6. Protobuf 的核心 API

### 6.1 序列化/反序列化

```cpp
// Person 对象的序列化方式（任选一种）：
std::string data;
p.SerializeToString(&data);          // → string（最常用）

std::vector<char> buf(p.ByteSizeLong());
p.SerializeToArray(buf.data(), buf.size());  // → char*

std::ostream os("person.bin", std::ios::binary);
p.SerializeToOstream(&os);           // → 文件流

// 反序列化
demo::Person p;
p.ParseFromString(data);             // ← 从 string
p.ParseFromArray(buf.data(), buf.size()); // ← 从 char*
```

### 6.2 常用 API

```cpp
demo::Person p;

// 写字段
p.set_id(42);                        // int32
p.set_name("Alice");                 // string
p.set_age(30);
p.set_email("alice@example.com");

// 读字段
int id = p.id();                     // getter（注意不是 getId）
const std::string& name = p.name();

// 判断是否有值（proto3 默认值不序列化）
if (p.has_email()) {
    // email 被设置了
}

// 清除字段
p.clear_name();

// 获取序列化后的大小（不实际序列化）
size_t sz = p.ByteSizeLong();
```

### 6.3 repeated（数组）字段

```protobuf
message Group {
    int32       group_id = 1;
    repeated    int32    member_ids = 2;   // 数组
    repeated    string   tags = 3;         // 字符串数组
}
```

```cpp
Group g;
g.set_group_id(1);
g.add_member_ids(100);
g.add_member_ids(101);
g.add_member_ids(102);
g.add_tags("cpp");
g.add_tags("network");

for (int i = 0; i < g.member_ids_size(); ++i) {
    std::cout << g.member_ids(i) << "\n";
}

// 遍历的另一种方式（C++11）：
for (const auto& tag : g.tags()) {
    std::cout << tag << "\n";
}
```

### 6.4 嵌套 message

```protobuf
message Address {
    string city    = 1;
    string street  = 2;
}

message Person {
    int32   id      = 1;
    string  name    = 2;
    Address address = 3;         // 嵌套另一个 message
    repeated string phones = 4;
}
```

```cpp
Person p;
p.set_id(42);
p.mutable_address()->set_city("Beijing");
p.mutable_address()->set_street("Zhongguancun");
p.add_phones("13800138000");
```

### 6.5 为什么不用自己调 htonl？

protobuf 对整数用了 **varint 编码**：

```cpp
// varint 编码规则：
// 小整数（0~127）：占 1 字节
// 大整数：占更多字节，但每字节的最高位是"是否还有后续"标志

// 值 42 的 varint 编码：  0x2A          ← 1 字节
// 值 300 的 varint 编码：  0xAC 0x02     ← 2 字节
// 值 100000 的 varint 编码：0xA0 0x8D 0x06 ← 3 字节
```

**varint 是小端序，但 protobuf 的序列化结果是平台无关的**——varint 编码/解码规则是固定的，不管你在 x86 还是 ARM 上跑，`SerializeToString` 出来的字节完全一样。所以**不需要调 `htonl`**。

---

## 7. CMake 入门与 protobuf 集成

### 7.1 CMake 是什么

CMake 是 C++ 项目的**构建系统生成器**。你写一个 `CMakeLists.txt` 描述"项目用哪些源文件、链接哪些库"，CMake 自动生成 Makefile 或 Visual Studio 项目文件。

**为什么不用手写 Makefile？**

```makefile
# Makefile：你得手动写每个 .o 的依赖
# 加一个 .cpp 就要改 Makefile

# CMake：只需描述一次
cmake_minimum_required(VERSION 3.20)
add_executable(server server.cpp session.cpp ...)
target_link_libraries(server protobuf asio)
```

### 7.2 最小的 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(ByteOrderDemo)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找 protobuf 库
find_package(protobuf REQUIRED)

# 编译 .proto 文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS person.proto)

# 生成可执行文件
add_executable(main ${PROTO_SRCS} ${PROTO_HDRS} main.cpp)
target_link_libraries(main protobuf::libprotobuf)
```

### 7.3 构建流程

```bash
mkdir build && cd build
cmake ..           # 读取 CMakeLists.txt，生成 Makefile
make               # 编译
./main             # 运行
```

### 7.4 CMake + protobuf 关键点

```cmake
# 告诉 CMake 找 protobuf 库
find_package(protobuf REQUIRED)
# REQUIRED = 必须找到，找不到报错退出

# 自动编译 .proto → .pb.h/.pb.cc
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS person.proto)
# 等价于执行：protoc --cpp_out=. person.proto

# 链接库
target_link_libraries(main protobuf::libprotobuf)
```

### 7.5 protobuf 版本问题

```bash
# 查看安装的 protobuf 版本
protoc --version
# 如果 protobuf_generate_cpp 不支持，用旧写法：
include(FindProtobuf)
find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS person.proto)
target_link_libraries(main ${PROTOBUF_LIBRARIES})
```

**CMake 新老写法对照**：

| 老写法（<3.18） | 新写法（≥3.18） |
|----------------|----------------|
| `include(FindProtobuf)` | `find_package(protobuf REQUIRED)` |
| `PROTOBUF_GENERATE_CPP(...)` | `protobuf_generate_cpp(...)` |
| `${PROTOBUF_LIBRARIES}` | `protobuf::libprotobuf` |

---

## 8. Protobuf + TLV 粘包方案——网络编程实战

### 8.1 完整协议栈

之前你从 base_tcp_msg.md 里学到的 TLV 粘包方案，加上 protobuf 后变成：

```
[ 总长度: 4B ][ MessageType: 2B ][ Protobuf 序列化数据: N B ]
   htonl          htons               protobuf 自动处理字节序
   ↑ TLV 粘包     ↑ 消息类型           ↑ 序列化内容
```

**各层的职责**：

```
TCP 字节流（内核）
  │
  ▼
粘包层：读 6B → 得到 type + body_len → 读 body_len 字节 → 完整消息
  │
  ▼
序列化层：ParseFromArray(body_data, body_len) → 得到结构体
  │
  ▼
业务层：使用结构体
```

### 8.2 .proto 文件

```protobuf
// chat.proto
syntax = "proto3";
package chat;

message LoginRequest {
    string username = 1;
    string password = 2;
}

message LoginResponse {
    bool   success = 1;
    string message = 2;
}

message ChatMessage {
    enum Type {
        TEXT = 0;
        IMAGE = 1;
        FILE = 2;
    };
    Type   type     = 1;
    int32  sender   = 2;
    int32  receiver = 3;
    string content  = 4;
}
```

### 8.3 服务端核心代码

```cpp
#include <boost/asio.hpp>
#include "chat.pb.h"
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

constexpr size_t HEADER_SIZE = 6;  // total_len(4) + msg_type(2)

// ===== 接收并解析一条 protobuf 消息 =====
template<typename T>
bool recv_protobuf(tcp::socket& sock, T& message) {
    // 1. 读 6 字节 header（解粘包）
    std::array<char, HEADER_SIZE> header;
    asio::read(sock, asio::buffer(header));

    uint32_t net_total = *(uint32_t*)header.data();
    uint32_t total     = ntohl(net_total);

    uint16_t net_type  = *(uint16_t*)(header.data() + 4);
    uint16_t msg_type  = ntohs(net_type);

    if (total > 10 * 1024 * 1024) {  // 防攻击
        return false;
    }

    // 2. 读 body_len 字节（protobuf 序列化数据）
    uint32_t body_len = total - HEADER_SIZE;
    std::string raw(body_len, '\0');
    asio::read(sock, asio::buffer(raw));

    // 3. 反序列化 protobuf 消息
    return message.ParseFromString(raw);
}

// ===== 发送一条 protobuf 消息 =====
template<typename T>
void send_protobuf(tcp::socket& sock, uint16_t msg_type,
                   const T& message) {
    // 1. 序列化 protobuf
    std::string body;
    message.SerializeToString(&body);

    // 2. 拼 TLV header
    uint32_t total = HEADER_SIZE + body.size();
    uint32_t net_total = htonl(total);
    uint16_t net_type  = htons(msg_type);

    std::array<asio::const_buffer, 3> bufs = {
        asio::buffer(&net_total, 4),
        asio::buffer(&net_type, 2),
        asio::buffer(body)
    };
    asio::write(sock, bufs);
}

// ===== 使用示例（echo 服务器回调） =====
void on_message(tcp::socket& sock) {
    // 读 header，根据 msg_type 决定反序列化类型
    chat::ChatMessage msg;
    if (recv_protobuf(sock, msg)) {
        std::cout << "Received: " << msg.content() << "\n";

        // echo 回去
        send_protobuf(sock, 2, msg);
    }
}
```

### 8.4 protobuf 对比手动序列化

| 场景 | 手动 `htonl` + `memcpy` | Protobuf |
|------|-------------------------|----------|
| Person 有 10 个字段 | `htonl` × 10 + `memcpy` × 10 | 自动 |
| 加了一个字段 | 通知所有人改代码 | 旧代码自动忽略未知字段 |
| 跨语言 | 用 Python 客户端？重写序列化 | `protoc --python_out` 即可 |
| 变长数据 | 固定数组浪费或 `vector` + 手写序列化 | `repeated` / `string` 天然支持 |
| 性能损失 | 0（直接内存拷贝） | 约 5~10%（varint 解码开销） |

### 8.5 完整 CMakeLists.txt（protobuf + ASIO）

```cmake
cmake_minimum_required(VERSION 3.20)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找 protobuf
find_package(protobuf REQUIRED)

# 找 Boost（ASIO）
find_package(Boost REQUIRED COMPONENTS system)

# 编译 .proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS chat.proto)

# 服务端
add_executable(server
    ${PROTO_SRCS} ${PROTO_HDRS}
    server.cpp
)
target_link_libraries(server
    protobuf::libprotobuf
    Boost::system
    pthread
)

# 客户端
add_executable(client
    ${PROTO_SRCS} ${PROTO_HDRS}
    client.cpp
)
target_link_libraries(client
    protobuf::libprotobuf
    Boost::system
    pthread
)
```

**构建步骤**：

```bash
cd project_dir
mkdir build && cd build
cmake ..
make
./server &
./client
```

---

## 9. 总结

### 9.1 Protobuf 的核心价值

```
手写序列化：定义结构体 → 手动 htonl → 手动 memcpy → 拼 buffer → send
                ↑ 每次加字段都要改

Protobuf：    写 .proto    → protoc 生成代码 → 一行 SerializeToString
                ↑ 加字段就加一行，重新 protoc 即可
```

### 9.2 之前内容 + protobuf 的完整链路

```
TCP 流                   应用层协议                序列化内容
┌──────────────────┐    ┌────────────────┐    ┌────────────────┐
│  sock.async_read │ ──→│  总长度(4B)    │ ──→│ protobuf 编码  │
│  解决粘包         │    │  消息类型(2B)   │    │ 自动字节序     │
│                  │    │  body(N B)     │    │ 跨语言兼容     │
└──────────────────┘    └────────────────┘    └────────────────┘
   base_tcp_msg.md        base_tcp_msg.md        base_protobuf.md
   base_byteorder.md

   ASIO                   CMake 组织项目
   base_asio.md
```

### 9.3 学习路径建议

```
1. 写一个 .proto → protoc → C++ 类（本节）
2. 集成到之前的 echo server 中（protobuf 作为 body 内容）
3. 添加上一章的 CMake 构建
4. 然后尝试 repeated、嵌套 message、oneof
5. 最后了解 proto3 vs proto2 的差异、gRPC（protobuf 的 RPC 框架）
```

### 9.4 一句话总结

> **Protobuf = 你结构体的自动序列化工具。它不解决粘包（靠长度前缀），不解决字节序（varint 自动处理），只管"内存 ↔ 字节流"。CMake 帮你管理 protoc 生成的代码和链接。**

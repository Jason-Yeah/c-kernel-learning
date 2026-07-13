# JsonCpp——C++ 中的 JSON 解析与序列化

> **目标读者**：写过网络程序，需要在 C++ 中解析或生成 JSON 数据；熟悉 protobuf 但发现有些场景 JSON 更合适；想知道 JsonCpp 怎么用、怎么和之前讲的网络编程结合。
>
> **与之前内容的关系**：protobuf 是二进制序列化，JSON 是文本序列化。在网络编程中，JSON 常用于 HTTP API、配置文件、日志输出。JsonCpp 是 C++ 中最流行的 JSON 库之一。

---

## 目录

1. [JSON 与 protobuf 的对比](#1-json-与-protobuf-的对比)
2. [JsonCpp 是什么](#2-jsoncpp-是什么)
3. [安装](#3-安装)
4. [快速上手——解析与生成](#4-快速上手解析与生成)
5. [JsonCpp 核心 API](#5-jsoncpp-核心-api)
6. [CMake 集成](#6-cmake-集成)
7. [网络编程实战——JSON echo server](#7-网络编程实战json-echo-server)
8. [常见陷阱](#8-常见陷阱)
9. [总结](#9-总结)

---

## 1. JSON 与 protobuf 的对比

### 1.1 两种序列化的定位

```
protobuf ← 机器间通信（高效、紧凑、需预定义 schema）
JSON     ← 人机交互（可读、灵活、无需预定义 schema）
```

**什么时候用 JSON：**

| 场景 | 为什么用 JSON |
|------|-------------|
| HTTP REST API | 前端/后端通用，浏览器原生支持 |
| 配置文件 | 可读可写，方便手动编辑 |
| 日志输出 | 结构化日志，方便工具解析 |
| 调试/测试 | 直接看到数据内容，不用 protobuf 反序列化 |
| 快速原型 | 不用写 .proto 文件，不用编译，随时改字段 |

**什么时候用 protobuf：**

| 场景 | 为什么用 protobuf |
|------|------------------|
| 内部 RPC（gRPC） | 性能好、带宽省、schema 强约束 |
| 高性能场景 | 解析比 JSON 快 3~10 倍 |
| 长期存储 | 字段编号不变就兼容 |
| 跨语言服务间通信 | 自动生成各语言代码 |

### 1.2 直观对比

```json
// JSON 版本（可读，但更长）
{"id": 42, "name": "Alice", "age": 30}

// protobuf 编码后（不可读，但更短）
// 0x08 0x2A 0x12 0x05 0x41 0x6C 0x69 0x63 0x65 0x18 0x1E
```

```
对比维度           JSON                protobuf
是否可读             ✅ 纯文本            ❌ 二进制
消息大小             大（含字段名）        小（只有字段编号）
解析性能             慢（字符串解析）      快（二进制解码）
需要 schema(.proto)  ❌ 不需要            ✅ 必须
泛型容器             ✅ 天然支持           ❌ 需定义 repeated
第三方依赖           一个头文件             protoc + 运行时库
```

---

## 2. JsonCpp 是什么

JsonCpp 是一个**纯 C++ 的 JSON 解析与生成库**，只由一个头文件（`json/json.h`）和一个源文件（`jsoncpp.cpp`）组成，或者链接 `libjsoncpp`。

```
输入 JSON 字符串：
    {"name":"Alice","age":30,"tags":["cpp","network"]}

JsonCpp 解析后：
    Json::Value root;           ← 一个"万能"容器
    root["name"] = "Alice";     ← 可以像 map 一样读写
    root["age"]  = 30;
    root["tags"][0] = "cpp";
```

**JsonCpp 的核心思想**：所有数据存在 `Json::Value` 里，它可以是：

| JSON 类型 | Json::Value 表示 | C++ 对应 |
|-----------|-----------------|---------|
| `null` | `isNull()` | `nullptr` |
| `true/false` | `isBool()` | `bool` |
| `42` | `isInt()` | `int` |
| `3.14` | `isDouble()` | `double` |
| `"hello"` | `isString()` | `std::string` |
| `[1,2,3]` | `isArray()` | `std::vector` |
| `{"k":"v"}` | `isObject()` | `std::map` |

**对比 protobuf 的思维转变**：

```cpp
// protobuf：先定义 message，然后像 struct 一样用字段
Person p;
p.set_id(42);
p.SerializeToString(&wire);

// JsonCpp：没有固定结构，运行时动态构造
Json::Value p;
p["id"] = 42;                    // 不需要预定义！
p["name"] = "Alice";             // 想加就加
std::string json = p.toStyledString();  // 格式化输出
```

---

## 3. 安装

### 3.1 Ubuntu/WSL

```bash
sudo apt install libjsoncpp-dev
```

### 3.2 验证安装

```bash
# 头文件
ls /usr/include/json/json.h

# 库文件（两个名字都可能存在）
ls /usr/lib/x86_64-linux-gnu/libjsoncpp*
```

### 3.3 编译测试

```bash
g++ -std=c++17 test.cpp -ljsoncpp -o test
```

---

## 4. 快速上手——解析与生成

### 4.1 解析 JSON 字符串

```cpp
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>

int main() {
    // 一段 JSON 字符串（可能是从网络收到的）
    std::string text = R"({
        "id": 42,
        "name": "Alice",
        "age": 30,
        "tags": ["cpp", "network"],
        "address": {
            "city": "Beijing",
            "street": "Zhongguancun"
        }
    })";

    // 1. 解析
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream iss(text);

    if (!Json::parseFromStream(reader, iss, &root, &errs)) {
        std::cerr << "parse error: " << errs << "\n";
        return 1;
    }

    // 2. 读取字段
    int id      = root["id"].asInt();
    std::string name = root["name"].asString();
    int age     = root["age"].asInt();

    std::cout << "id=" << id << ", name=" << name << ", age=" << age << "\n";

    // 3. 读取数组
    for (const auto& tag : root["tags"]) {
        std::cout << "tag: " << tag.asString() << "\n";
    }

    // 4. 读取嵌套对象
    std::string city = root["address"]["city"].asString();
    std::cout << "city: " << city << "\n";

    return 0;
}
```

### 4.2 生成 JSON

```cpp
#include <json/json.h>
#include <iostream>

int main() {
    Json::Value root;

    // 1. 填充字段
    root["id"]   = 42;
    root["name"] = "Alice";
    root["age"]  = 30;

    // 2. 构造数组
    Json::Value tags(Json::arrayValue);
    tags.append("cpp");
    tags.append("network");
    tags.append("json");
    root["tags"] = tags;

    // 3. 构造嵌套对象
    Json::Value addr;
    addr["city"]   = "Beijing";
    addr["street"] = "Zhongguancun";
    root["address"] = addr;

    // 4. 输出
    // 紧凑格式 —— 适合网络传输
    Json::StreamWriterBuilder writer;
    std::string compact = Json::writeString(writer, root);
    std::cout << "compact:  " << compact << "\n";

    // 格式化输出 —— 适合阅读/日志
    // toStyledString 是老 API，默认带缩进
    std::cout << "pretty:\n" << root.toStyledString();

    return 0;
}
```

---

## 5. JsonCpp 核心 API

### 5.1 构造与类型判断

```cpp
Json::Value v;                   // null

v = true;                        // bool
v = 42;                          // int
v = 3.14;                        // double
v = "hello";                     // string
v = Json::Value(Json::arrayValue);  // 空数组
v = Json::Value(Json::objectValue); // 空对象

// 类型检查
if (v.isNull())      {}
if (v.isBool())      {}
if (v.isInt())       {}
if (v.isDouble())    {}
if (v.isString())    {}
if (v.isArray())     {}
if (v.isObject())    {}
```

### 5.2 读值

```cpp
Json::Value root = parse(json_str);

// 安全读（带默认值）——推荐
int id = root.get("id", 0).asInt();              // 没有 id 字段则返回 0
std::string name = root.get("name", "").asString();
int age = root.get("age", -1).asInt();

// 不安全读——如果字段不存在返回 Json::nullValue
int val = root["nonexist"].asInt();              // 返回 0，不会崩
// 但访问数组越界会崩：
Json::Value arr(Json::arrayValue);
arr[0] = "first";
std::cout << arr[5];    // 越界！可能崩
```

### 5.3 遍历数组

```cpp
Json::Value arr = root["tags"];

// 方式一：范围 for（C++11，推荐）
for (const auto& item : arr) {
    std::cout << item.asString() << "\n";
}

// 方式二：下标
for (unsigned i = 0; i < arr.size(); ++i) {
    std::cout << arr[i].asString() << "\n";
}
```

### 5.4 遍历对象

```cpp
Json::Value obj = root["address"];

// 遍历所有键值对
for (auto it = obj.begin(); it != obj.end(); ++it) {
    std::string key   = it.key().asString();   // 字段名
    Json::Value value = *it;                    // 字段值
    std::cout << key << " = " << value << "\n";
}
```

### 5.5 删除与修改

```cpp
// 添加或修改
root["new_field"] = "new_value";

// 删除
root.removeMember("old_field");

// 清空
root.clear();
```

### 5.6 序列化输出（两种 API）

```cpp
// ===== 新 API（推荐）=====
Json::StreamWriterBuilder builder;
builder["indentation"] = "    ";          // 4 空格缩进
std::string pretty  = Json::writeString(builder, root);

builder["indentation"] = "";              // 无缩进=紧凑
std::string compact = Json::writeString(builder, root);

// ===== 老 API（简单但少选项）=====
std::string styled = root.toStyledString();  // 默认漂亮打印
```

---

## 6. CMake 集成

### 6.1 基本配置

```cmake
cmake_minimum_required(VERSION 3.20)
project(JsonDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找 jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

include_directories(${JSONCPP_INCLUDE_DIRS})

add_executable(demo main.cpp)
target_link_libraries(demo ${JSONCPP_LIBRARIES})
```

或者（如果 jsoncpp 有 cmake config）：

```cmake
find_package(jsoncpp REQUIRED)
target_link_libraries(demo jsoncpp_lib)
```

### 6.2 完整示例（结合 ASIO）

```cmake
cmake_minimum_required(VERSION 3.20)
project(JsonEcho)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找 jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
include_directories(${JSONCPP_INCLUDE_DIRS})

# 找 Boost/ASIO
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# 可执行文件
add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server ${JSONCPP_LIBRARIES} pthread)

add_executable(echo_client echo_client.cpp)
target_link_libraries(echo_client ${JSONCPP_LIBRARIES} pthread)
```

### 6.3 编译验证

```bash
g++ -std=c++17 main.cpp -ljsoncpp -o main
./main
```

---

## 7. 网络编程实战——JSON echo server

### 7.1 协议设计

直接用 JSON 字符串作为消息体，用 `\n`（换行符）作为消息边界：

```
TCP 字节流：
    {"type":"chat","sender":1001,"content":"hello"}\n
    {"type":"heartbeat","ts":1700000000}\n
```

### 7.2 Server 端

```cpp
#include <boost/asio.hpp>
#include <json/json.h>
#include <iostream>
#include <memory>
#include <sstream>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket sock) : _sock(std::move(sock)) {}
    void start() { read_line(); }

private:
    void read_line() {
        auto self = shared_from_this();
        // 读到 \n 为止 —— 每行一个 JSON 对象
        asio::async_read_until(_sock, _streambuf, '\n',
            [self](boost::system::error_code ec, size_t) {
                if (ec) return;
                self->on_line();
            });
    }

    void on_line() {
        // 从 streambuf 取出一行
        std::istream is(&_streambuf);
        std::string line;
        std::getline(is, line);  // 读到 \n 为止

        // 解析 JSON
        Json::Value root;
        Json::CharReaderBuilder reader;
        std::istringstream iss(line);
        std::string errs;

        if (!Json::parseFromStream(reader, iss, &root, &errs)) {
            std::cerr << "json parse error: " << errs << "\n";
            read_line();
            return;
        }

        // 处理消息
        std::string type = root.get("type", "").asString();
        if (type == "chat") {
            int sender  = root["sender"].asInt();
            std::string content = root["content"].asString();
            std::cout << "[chat] " << sender << ": " << content << "\n";

            // echo 回去（加上换行符）
            root["echo"] = true;
            Json::StreamWriterBuilder writer;
            std::string response = Json::writeString(writer, root) + "\n";
            asio::async_write(_sock, asio::buffer(response),
                [self = shared_from_this()](ec, size_t) {
                    self->read_line();
                });
        }
    }

    tcp::socket _sock;
    asio::streambuf _streambuf;
};

// Server 启动略（跟之前一样，创建 acceptor → async_accept → Session）
```

### 7.3 客户端示例

```cpp
#include <boost/asio.hpp>
#include <json/json.h>
#include <iostream>
#include <sstream>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

int main() {
    try {
        asio::io_context io;
        tcp::socket sock(io);
        tcp::resolver resolver(io);
        asio::connect(sock, resolver.resolve("127.0.0.1", "9192"));

        // 构造 JSON 消息
        Json::Value msg;
        msg["type"]    = "chat";
        msg["sender"]  = 1001;
        msg["content"] = "Hello from JSON client!";

        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, msg) + "\n";
        asio::write(sock, asio::buffer(payload));

        // 接收 echo 回复
        asio::streambuf buf;
        asio::read_until(sock, buf, '\n');

        std::istream is(&buf);
        std::string reply_line;
        std::getline(is, reply_line);

        Json::Value reply;
        Json::CharReaderBuilder reader;
        std::istringstream iss(reply_line);
        if (Json::parseFromStream(reader, iss, &reply, nullptr)) {
            std::cout << "echo: " << reply.toStyledString();
        }

        sock.close();
    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
    }
    return 0;
}
```

### 7.4 JSON vs protobuf 在网络编程中的选择

```
JSON 适合：
  - REST API、WebSocket 文本协议
  - 调试/快速原型
  - 前-后端通信（浏览器不用 protobuf）
  - 不需要强 schema 的场景

protobuf 适合：
  - 高性能内部 RPC（gRPC）
  - 带宽敏感场景
  - 需要强 schema 约束
  - C++↔C++ 或自动生成代码的场景

两者不冲突：
  你可以 protobuf 做服务间通信
       JSON 做外部 API 和日志
```

---

## 8. 常见陷阱

### 8.1 `asInt()` 不检查类型

```cpp
Json::Value v = "hello";
int n = v.asInt();     // 返回 0，不会报错也不抛异常
// 应先用 isInt() 检查
if (v.isInt()) {
    int n = v.asInt();
}
```

### 8.2 数组越界不报错

```cpp
Json::Value arr(Json::arrayValue);
arr[0] = "a";
arr[1] = "b";
arr[5] = "c";    // 不会报错！arr[2][3][4] 被自动填充为 null
```

### 8.3 浮点数精度

```cpp
Json::Value v;
v["pi"] = 3.14159265358979;
// JSON 标准中浮点数用 double，精度有限，不要用 JSON 传高精度小数
```

### 8.4 大 JSON 性能

```cpp
// JsonCpp 解析 100MB JSON 可能很慢
// 稳定方案：换 RapidJSON（更快）或 simdjson（极致性能）
```

---

## 9. 总结

### 9.1 JsonCpp 速查

| 操作 | 代码 |
|------|------|
| 解析字符串 | `Json::parseFromStream(reader, iss, &root, &errs)` |
| 构造对象 | `root["key"] = value` |
| 构造数组 | `root["arr"].append(value)` |
| 读值 | `root.get("key", default).asString()` |
| 遍历数组 | `for (const auto& v : arr)` |
| 遍历对象 | `for (auto it = obj.begin(); it != obj.end(); ++it)` |
| 序列化（紧凑） | `Json::writeString(builder, root)` |
| 序列化（漂亮） | `root.toStyledString()` |
| CMake | `pkg_check_modules(JSONCPP REQUIRED jsoncpp)` |

### 9.2 与之前内容的完整链路

```
base_tcp_msg.md      base_byteorder.md     base_protobuf.md      base_jsoncpp.md
┌─────────────┐      ┌────────────┐        ┌──────────────┐      ┌────────────┐
│ 粘包解决      │      │ 字节序处理  │        │ protobuf 序列化│      │ JSON 序列化 │
│ 分隔符/长度前缀│      │ htonl/ntohl│        │ 二进制、高效  │      │ 文本、可读  │
│              │      │            │        │ 需 .proto    │      │ 无需 schema │
└──────┬──────┘      └──────┬─────┘        └──────┬───────┘      └─────┬──────┘
       │                    │                      │                   │
       ▼                    ▼                      ▼                   ▼
       └──────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
                            你的应用层消息体
                            TLV + 序列化内容
```

### 9.3 一句话

> **protobuf 给机器看（高效、紧凑），JSON 给人看（可读、灵活）。JsonCpp 是 C++ 里最省事的 JSON 库——装了就忘不掉。**

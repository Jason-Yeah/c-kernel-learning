# C++ 流（iostream）全面梳理

## 1. 什么是流

**流（Stream）** 是 C++ 中用于处理输入输出的抽象概念，它将数据表示为一个**字节序列**，屏蔽了底层设备（键盘、文件、内存、网络）的差异。

```
数据源  →  [ 缓冲区 ]  →  程序
程序   →  [ 缓冲区 ]  →  数据目标
```

### 核心特性

| 特性 | 说明 |
|---|---|
| **类型安全** | 编译器知道类型，不会像 `printf` 那样类型不匹配 |
| **可扩展** | 用户自定义类型可以重载 `<<` / `>>` |
| **有状态** | 格式标志、精度、宽度等状态随流携带 |
| **缓冲** | 大部分流有缓冲区，减少系统调用 |
| **错误可检** | 通过 `eof()` / `fail()` / `bad()` / `good()` 检查状态 |

---

## 2. 流类层次结构

```
                            ios_base
                               |
                              ios
                            /     \
                       istream   ostream
                         \       /
                        iostream
                       /    |    \
              ifstream   fstream   ofstream
              istringstream   ostringstream / stringstream
```

### 2.1 标准库主要流类

| 类 | 头文件 | 作用 |
|---|---|---|
| `std::ios_base` | `<ios>` | 所有流基类，含格式标志、错误状态 |
| `std::ios` | `<ios>` | 继承 `ios_base`，管理缓冲区关联 |
| `std::istream` | `<istream>` | 输入流基类 |
| `std::ostream` | `<ostream>` | 输出流基类 |
| `std::iostream` | `<iostream>` | 双向流基类 |
| `std::ifstream` | `<fstream>` | 文件输入流 |
| `std::ofstream` | `<fstream>` | 文件输出流 |
| `std::fstream` | `<fstream>` | 文件双向流 |
| `std::istringstream` | `<sstream>` | 字符串输入流 |
| `std::ostringstream` | `<sstream>` | 字符串输出流 |
| `std::stringstream` | `<sstream>` | 字符串双向流 |
| `std::cin` / `std::cout` / `std::cerr` / `std::clog` | `<iostream>` | 标准输入/输出/错误/日志 |

### 2.2 标准预定义流对象

| 对象 | 关联设备 | 缓冲类型 | 用途 |
|---|---|---|---|
| `cin` | 标准输入（键盘） | 行缓冲（与 `stdio` 同步时） | 读取用户输入 |
| `cout` | 标准输出（终端） | 行缓冲 | 普通输出 |
| `cerr` | 标准错误 | 无缓冲 | 错误输出，立即刷新 |
| `clog` | 标准错误 | 有缓冲 | 日志输出 |
| `wcin` / `wcout` / `wcerr` / `wclog` | 宽字符版本 | 同上 | 处理 `wchar_t` |

---

## 3. 基本用法

### 3.1 输出 `<<`

```cpp
#include <iostream>
#include <iomanip>

int main() {
    int n = 42;
    double pi = 3.1415926535;
    std::string s = "hello";

    std::cout << "n = " << n << '\n'
              << "pi = " << pi << '\n'
              << "s = " << s << std::endl;   // endl = '\n' + flush
}
```

### 3.2 输入 `>>`

```cpp
int age;
std::string name;

std::cout << "Enter age: ";
std::cin >> age;                    // 跳过空白，读一个 int

std::cout << "Enter name: ";
std::cin >> name;                   // 跳过空白，读到下一个空白为止

std::cin.ignore();                  // 忽略换行符
std::getline(std::cin, name);       // 读一整行（含空格）
```

> **注意：** `>>` 会跳过前导空白，遇到空白停止；`std::getline` 读整行（包括空格），遇到 `\n` 停止。

### 3.3 错误状态

```cpp
if (std::cin >> value) {
    // 读取成功
} else {
    if (std::cin.eof())  { /* 文件结束 */ }
    if (std::cin.fail()) { /* 格式错误（如输入"abc"给int）*/ }
    if (std::cin.bad())  { /* 流损坏（不可恢复）*/ }

    std::cin.clear();                  // 重置状态
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空缓冲区
}
```

| 状态位 | 含义 | 恢复方式 |
|---|---|---|
| `goodbit` (0) | 一切正常 | — |
| `eofbit` (1) | 读到文件末尾 | `clear()` 清除 |
| `failbit` (2) | 格式错误（可恢复） | `clear()` 后重试 |
| `badbit` (4) | 流损坏（不可恢复） | 通常只能销毁流 |

---

## 4. 格式化 I/O

### 4.1 流操纵器（Manipulators）

| 操纵器 | 头文件 | 作用 |
|---|---|---|
| `std::boolalpha` / `std::noboolalpha` | `<iostream>` | bool 输出 true/false 或 1/0 |
| `std::hex` / `std::dec` / `std::oct` | `<iostream>` | 整数进制 |
| `std::showbase` / `std::noshowbase` | `<iostream>` | 显示进制前缀（0x / 0） |
| `std::uppercase` / `std::nouppercase` | `<iostream>` | 十六进制大写字母 |
| `std::fixed` / `std::scientific` / `std::defaultfloat` | `<iostream>` | 浮点格式 |
| `std::setprecision(n)` | `<iomanip>` | 设置浮点精度 |
| `std::setw(n)` | `<iomanip>` | 设置下次输出的最小宽度 |
| `std::setfill(c)` | `<iomanip>` | 设置填充字符 |
| `std::left` / `std::right` / `std::internal` | `<iostream>` | 对齐方式 |
| `std::showpos` / `std::noshowpos` | `<iostream>` | 正数是否显示 `+` |
| `std::skipws` / `std::noskipws` | `<iostream>` | 输入是否跳过空白 |
| `std::flush` | `<iostream>` | 刷新缓冲区 |
| `std::endl` | `<iostream>` | 输出 `\n` 并刷新 |

### 4.2 示例

```cpp
#include <iostream>
#include <iomanip>

int main() {
    // ----- bool -----
    std::cout << std::boolalpha << true << " " << false << '\n';  // true false

    // ----- 进制 -----
    std::cout << std::showbase
              << std::hex << 255 << " "        // 0xff
              << std::dec << 255 << " "        // 255
              << std::oct << 255 << '\n';      // 0377

    // ----- 浮点 -----
    double d = 3.1415926535;
    std::cout << std::fixed << std::setprecision(4) << d << '\n';  // 3.1416
    std::cout << std::scientific << std::setprecision(6) << d << '\n'; // 3.141593e+00

    // ----- 对齐与宽度 -----
    std::cout << std::left << std::setw(10) << "name"
              << std::right << std::setw(5) << "age" << '\n';
    std::cout << std::left << std::setw(10) << "Alice"
              << std::right << std::setw(5) << 30 << '\n';
    // name       age
    // Alice       30
}
```

### 4.3 持久性

| 操纵器 | 持久性 |
|---|---|
| `std::setw(n)` | **仅影响下一笔输出** |
| `std::setprecision(n)` | 持久，直到被改变 |
| `std::boolalpha` / `std::hex` / `std::fixed` 等 | 持久，直到被改变 |

---

## 5. 文件流

### 5.1 基本文件操作

```cpp
#include <fstream>
#include <string>

// ----- 写文件 -----
std::ofstream ofs("data.txt");
if (!ofs) {
    std::cerr << "Failed to open file for writing\n";
    return 1;
}
ofs << "Hello, file!\n" << 42 << '\n';
ofs.close();

// ----- 读文件 -----
std::ifstream ifs("data.txt");
std::string line;
while (std::getline(ifs, line)) {
    std::cout << line << '\n';
}
ifs.close();

// ----- 追加模式 -----
std::ofstream app("log.txt", std::ios::app);
app << "[" << __TIME__ << "] event logged\n";
```

### 5.2 打开模式

| 标志 | 含义 |
|---|---|
| `std::ios::in` | 读模式（默认 for `ifstream`） |
| `std::ios::out` | 写模式（默认 for `ofstream`） |
| `std::ios::app` | 追加（每次写都在末尾） |
| `std::ios::ate` | 打开后定位到末尾 |
| `std::ios::trunc` | 打开时清空文件 |
| `std::ios::binary` | 二进制模式（不转换换行符） |

```cpp
// 常用组合
std::fstream fs("data.bin", std::ios::in | std::ios::out | std::ios::binary);
std::ofstream ofs("data.txt", std::ios::out | std::ios::trunc);  // 覆盖写入
```

### 5.3 随机访问

```cpp
std::ifstream ifs("data.bin", std::ios::binary);
ifs.seekg(0, std::ios::end);                // 定位到末尾
auto size = ifs.tellg();                     // 获取文件大小
ifs.seekg(0, std::ios::beg);                // 回到开头

char buf[256];
ifs.read(buf, sizeof(buf));                  // 读固定字节
ifs.read(reinterpret_cast<char*>(&some_struct), sizeof(some_struct));
```

| 函数 | 输入流 | 输出流 |
|---|---|---|
| 定位 | `seekg(offset, dir)` | `seekp(offset, dir)` |
| 当前位置 | `tellg()` | `tellp()` |
| 基准 | `beg` / `cur` / `end` | `beg` / `cur` / `end` |

---

## 6. 字符串流

```cpp
#include <sstream>

// ----- 拼接字符串 -----
std::ostringstream oss;
oss << "value = " << 42 << ", pi = " << 3.14;
std::string result = oss.str();          // "value = 42, pi = 3.14"

// ----- 解析字符串 -----
std::istringstream iss("42 3.14 hello");
int n; double d; std::string s;
iss >> n >> d >> s;                       // n=42, d=3.14, s="hello"

// ----- 类型转换工具（替代 std::to_string 的灵活方式）-----
template <typename T>
std::string to_string_custom(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// ----- 清空重用 -----
std::ostringstream oss2;
oss2 << "first";
oss2.str("");                            // 重置内容
oss2.clear();                            // 重置状态位
oss2 << "second";
```

---

## 7. 自定义类型重载 `<<` 和 `>>`

```cpp
struct Point {
    int x, y;
};

// 输出
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << '(' << p.x << ", " << p.y << ')';
}

// 输入
std::istream& operator>>(std::istream& is, Point& p) {
    char ch1, ch2, ch3;
    if (is >> ch1 >> p.x >> ch2 >> p.y >> ch3) {
        if (ch1 != '(' || ch2 != ',' || ch3 != ')') {
            is.setstate(std::ios::failbit);  // 格式错误
        }
    }
    return is;
}

// 使用
Point p{10, 20};
std::cout << p << '\n';                   // (10, 20)

std::istringstream("(30, 40)") >> p;      // p = {30, 40}
```

### 设计要点

- 返回 `std::ostream&` / `std::istream&` 以支持链式调用
- 不要修改流的格式状态（或记得恢复），否则会"泄漏"给调用方
- 输入时遇到格式错误应设置 `failbit` 而非直接抛异常

---

## 8. 缓冲区与性能

### 8.1 同步与解同步

C++ 的 `cin` / `cout` 默认与 C 的 `stdin` / `stdout` 同步，保证 `printf` 和 `cout` 混用时顺序正确。但这会带来性能损失。

```cpp
#include <iostream>

// 关闭与 stdio 的同步（cin/cout 混用 printf/scanf 时不要这么做）
std::ios::sync_with_stdio(false);

// 解除 cin 和 cout 的绑定（cin 读取前不再自动刷新 cout）
std::cin.tie(nullptr);
```

**性能对比：**

```cpp
// 慢（默认同步）
for (int i = 0; i < 1'000'000; ++i)
    std::cout << i << '\n';

// 快（解同步 + '\n' 代替 endl）
std::ios::sync_with_stdio(false);
for (int i = 0; i < 1'000'000; ++i)
    std::cout << i << '\n';
```

> **第一条原则：** `'\n'` 代替 `endl`，除非你真的需要立即刷新。

### 8.2 直接缓冲区操作

```cpp
std::ostringstream oss;
oss << "many " << "small " << "pieces " << "data ";
// 最终一次取结果，避免多次刷新
std::string result = oss.str();
```

### 8.3 `rdbuf()` — 直接操作流缓冲区

```cpp
// 文件拷贝（通过缓冲区直接传输，避免逐行读写）
std::ifstream src("source.dat", std::ios::binary);
std::ofstream dst("dest.dat", std::ios::binary);
dst << src.rdbuf();          // 整个文件通过缓冲区拷贝

// 重定向 cout 到文件
std::ofstream file("log.txt");
auto old_buf = std::cout.rdbuf(file.rdbuf());
std::cout << "This goes to file" << '\n';
std::cout.rdbuf(old_buf);    // 恢复
```

---

## 9. 常见陷阱

### 9.1 `>>` 与 `getline` 混用

```cpp
int age;
std::string name;

std::cin >> age;              // 读数字，留下换行符在缓冲区
std::getline(std::cin, name); // ❌ 换行符还没被读取，getline 直接读到一个空行
```

**修复：**

```cpp
std::cin >> age;
std::cin.ignore();             // 消耗掉换行符
std::getline(std::cin, name);  // ✅
```

### 9.2 `eof()` 的误用

```cpp
// ❌ 常见错误：先检查 eof
while (!ifs.eof()) {          // 读到末尾时 eof 还没设置
    std::getline(ifs, line);
    std::cout << line << '\n'; // 最后一行会被多输出一次
}

// ✅ 正确写法
while (std::getline(ifs, line)) {
    std::cout << line << '\n';
}
```

### 9.3 `failbit` 的恢复

```cpp
int n;
std::cin >> n;                // 输入 "abc" → failbit 被设置

// ❌ 错误：不清除状态继续读
std::cin >> n;                // 立刻失败，因为 failbit 还在

// ✅ 正确
std::cin.clear();             // 清除 failbit
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 跳过坏输入
std::cin >> n;                // 重试
```

### 9.4 流对象的拷贝

```cpp
std::ofstream ofs("file.txt");
// std::ofstream copy = ofs;  // ❌ 流对象不可拷贝（拷贝构造函数 = delete）
std::ofstream& ref = ofs;     // ✅ 只能传引用
```

---

## 10. C++17/20 流相关新特性

### 10.1 `std::filesystem::path` 与流

```cpp
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
fs::path p = "data/out.txt";

if (!fs::exists(p.parent_path()))
    fs::create_directories(p.parent_path());

std::ofstream ofs(p);         // C++17: ofstream 可以直接接受 path
```

### 10.2 `std::span` 与流（C++20）

```cpp
#include <span>
#include <fstream>

char buf[1024];
std::ifstream ifs("data.bin", std::ios::binary);
ifs.read(buf, sizeof(buf));

std::span<char> data(buf, ifs.gcount());  // 只读实际读取的字节数
```

### 10.3 `std::format`（C++20）—— 流的替代方案

```cpp
#include <print>   // C++23
#include <format>  // C++20

// C++20: format（返回字符串）
std::string s = std::format("{} + {} = {}", 1, 2, 3);

// C++23: print（直接输出）
std::println("{} + {} = {}", 1, 2, 3);
```

`std::format` 相比 `<<` 的优势：

| | `<<` | `std::format` |
|---|---|---|
| 格式与参数分离 | ❌ 混在表达式中 | ✅ `"x={}, y={}"` 格式串独立 |
| 编译期检查 | ❌ 运行时才报错 | ✅ C++20 无，但 C++23 的 `std::print` 支持 |
| 性能 | 链式调用，多次函数 | 一次格式化，一次写入 |
| 国际化 | ❌ 困难 | ✅ 格式串可提取为资源 |

---

## 总结

| 场景 | 推荐 |
|---|---|
| 简单终端输出 | `std::cout << ...` |
| 格式化输出 | `std::format()` (C++20) / `std::println()` (C++23) |
| 文件读写 | `std::ifstream` / `std::ofstream` |
| 字符串拼接 | `std::ostringstream` |
| 字符串解析 | `std::istringstream` |
| 二进制 I/O | `read()` / `write()` + `std::ios::binary` |
| 高性能竞赛 / OI | `sync_with_stdio(false)` + `'\n'` |

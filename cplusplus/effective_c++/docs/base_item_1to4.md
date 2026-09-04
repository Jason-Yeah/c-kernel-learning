# Effective C++：条款 1～4 学习笔记

> 本文对应《Effective C++》（第三版）第一章 **Accustoming Yourself to C++（让自己习惯 C++）**。这四条不是零散技巧，而是在建立一个共同的编程习惯：先分清自己正在使用 C++ 的哪一部分，再用类型系统表达约束，并保证对象从诞生起就处于可用状态。
>
> 本书写于 C++11 之前。文中的核心原则依然适用；本文会在需要处补充现代 C++（C++11 及以后）的写法。示例默认需包含相应的标准库头文件。

---

## 本章路线图

| 条款 | 一句话记忆 | 解决的主要问题 |
| --- | --- | --- |
| 1 | C++ 是多种编程语言的联合体 | 同一段语法在不同语境下有不同成本与规则 |
| 2 | 用 `const`、`enum`、`inline` 替代宏 | 宏没有类型、作用域和调试信息 |
| 3 | 能写 `const` 就写 `const` | 用类型表达“不应修改”的约定 |
| 4 | 使用对象前先初始化 | 未初始化值与跨编译单元初始化顺序会导致未定义行为 |

学习顺序也很自然：条款 1 提醒我们不要过度概括规则；条款 2、3 用编译器可检查的方式描述“常量”和“只读”；条款 4 把这种“状态明确”的要求延伸到对象的初始状态。

---

## 条款 1：视 C++ 为一个语言联邦

### 核心结论

**不要把 C++ 当作只有一套规则的语言。** 它至少包含四种相互关联的子语言；同一个设计决定在不同子语言中的代价可能不同。

| 子语言 | 典型内容 | 应有的思考方式 |
| --- | --- | --- |
| C | 内置类型、数组、指针、预处理器 | 资源和内存布局更直接，容易踩未初始化、越界等问题 |
| 面向对象 C++ | 类、封装、继承、多态 | 关注对象不变量、虚函数、资源所有权 |
| 模板 C++ | 模板、泛型算法、概念（现代 C++） | 代码在实例化后才呈现真正类型；错误往往在调用点暴露 |
| STL | 容器、迭代器、算法、函数对象/lambda | 容器、迭代器、算法之间有各自的失效规则和复杂度承诺 |

### 为什么这很重要

例如“传值会产生一次拷贝”是一个不够精确的说法：

```cpp
int n = 42;                 // 内置类型：通常就是复制一个整数
std::string name = "Ada";   // 类类型：调用拷贝/移动构造，可能分配内存
std::vector<int> v = {1, 2}; // 容器：复制所有元素，成本与元素个数相关
```

因此不能从 `int` 的经验直接推导出 `std::vector<T>` 的性能结论。模板中尤其如此：`T` 到底是 `int`、大对象，还是带资源管理语义的类型，会改变最佳做法。

另一个例子是 `delete`：对 C 风格数组需要 `delete[]`，对单个对象使用 `delete`；而现代 C++ 中更好的默认选择是让 `std::vector`、`std::string`、`std::unique_ptr` 等 RAII 类型自动管理资源。这里同时涉及 C 子语言、面向对象 C++ 和 STL。

### 初学者的落地检查

看到一段 C++ 时，先问：

1. 这里处理的是内置值、对象、模板参数，还是 STL 组件？
2. 对象的复制、移动、析构和异常安全由谁负责？
3. 若使用容器与迭代器，操作后迭代器/引用/指针是否仍有效？
4. 这条“经验”是否只在某个具体类型或某个语言子集里成立？

### 易混点

- “语言联邦”不是说要把四部分割裂使用；它是提醒你根据语境选择规则。
- 现代 C++ 还可以把并发、范围库、协程等看作重要语境，但不改变本条的思考方式。
- 不要因害怕成本而一律传引用。先看类型、所有权和接口语义，再衡量性能；必要时再测量。

---

## 条款 2：尽量以 `const`、`enum`、`inline` 替换 `#define`

### 核心结论

宏是预处理阶段的**纯文本替换**，发生在编译器理解类型、作用域之前。能让编译器看见的常量、函数和类型，通常应优先让编译器看见。

### 为什么宏没有类型、也不遵守 C++ 作用域？

先把一次编译分成两个层次理解：

```text
第 1 层：预处理器处理 #include、#define、#if
        它只替换 token（记号），还不认识“变量”“函数”“namespace”“类型”。

第 2 层：C++ 编译器解析预处理后的结果
        此时才检查语法、作用域、类型、重载和模板。
```

例如：

```cpp
#define ASPECT_RATIO 1.653

namespace ui {
    double width = 100.0 * ASPECT_RATIO;
}
```

预处理后，编译器实际看到的近似是：

```cpp
namespace ui {
    double width = 100.0 * 1.653;
}
```

这里从来没有叫 `ASPECT_RATIO` 的 C++ 对象：它没有类型、没有地址、没有所属命名空间，也不能被调试器当作变量观察。展开出的字面量 `1.653` 本身默认是 `double`，但它能否、以及何时转换为周围表达式所需的类型，要由展开后的上下文决定；宏名本身不携带类型信息。

这就是“宏不遵守作用域”的准确含义：宏定义不属于 C++ 的 namespace、类或代码块作用域。即使把它写在 namespace 的花括号内，预处理器仍会把它视作当前翻译单元后续文本中的宏：

```cpp
namespace ui {
    #define TIMEOUT_MS 500  // 这不是 ui::TIMEOUT_MS
}

int timeout = TIMEOUT_MS;   // 仍会被展开；名称实际泄漏到这里
```

宏的有效范围是“从 `#define` 出现处到 `#undef` 或翻译单元结束”，并会受 `#include` 影响。因此一个公共头文件定义 `min`、`max`、`check` 之类宏，可能意外改写所有包含它的源文件；这正是大型工程中宏名冲突难定位的原因。

相对地，下面的常量是真正的 C++ 声明：

```cpp
namespace ui {
    inline constexpr double kAspectRatio = 1.653;
}

double width = 100.0 * ui::kAspectRatio;
```

编译器知道 `ui::kAspectRatio` 的类型是 `const double`、知道它属于 `ui` 命名空间，也能在报错信息和调试器中保留该名字。这不是“写法更漂亮”，而是把约束从人脑的约定交给了编译器验证。

### 函数宏为什么特别容易出错

函数宏也只是替换文本，并不是函数调用。先看一个没有足够括号的例子：

```cpp
#define SQUARE(x) x * x

int result = SQUARE(1 + 2); // 展开为：1 + 2 * 1 + 2，结果是 5，不是 9
```

即使补上括号，副作用问题依然存在：

```cpp
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int count = 3;
int largest = MAX(++count, 10);
// 条件 ++count > 10 为假时，count 自增一次；为真时，第二个 ++count 又执行一次。
// 传入一次实参，实际可能执行两次。
```

预处理器不会建立参数列表、不会进行一次求值、也不会检查“`a` 和 `b` 必须能比较且结果类型兼容”。这些事情只能在替换完成后，由编译器对那一大段展开文本检查；错误信息也往往指向展开后的代码，而不是一个真正可单步进入的函数。

函数/函数模板则具有真正的参数、返回类型和一次求值规则：

```cpp
template <class T>
constexpr const T& maxValue(const T& a, const T& b) {
    return a < b ? b : a;
}

int count = 3;
int largest = maxValue(++count, 10); // ++count 只在调用前求值一次
```

注意这里“只求值一次”来自**函数调用规则**，不是 `constexpr` 或 `inline` 魔法。函数模板仍要求 `T` 支持 `<`；若不支持，编译器会在模板实例化时给出类型相关的错误。上例返回引用的生命周期限制仍然存在，实际开发中优先考虑 `std::max`，并避免把其引用结果绑定到已销毁的临时对象。

### 1. 用具名常量替代对象宏

```cpp
// 不推荐：没有类型，不遵守作用域规则
#define ASPECT_RATIO 1.653

// 推荐：有类型、可被调试器识别、受命名空间约束
constexpr double kAspectRatio = 1.653;
```

`constexpr` 是现代 C++ 中表达“编译期常量”的首选。若只是“不能经此名字修改”，但值不必是编译期常量，则使用 `const`：

```cpp
const std::string kConfigFile = "app.conf";
```

#### `const` 与 `constexpr`：先建立最重要的区别

`const` 约束的是“**这个对象不能再被修改**”；`constexpr` 约束的是“**这个值（或函数调用）可以在编译期算出来**”。两者经常同时出现，但不是同义词。

```cpp
int readPortFromFile();

const int port = readPortFromFile(); // 可以：port 初始化后不能改，但读取文件发生在运行期
constexpr int secondsPerHour = 60 * 60; // 可以：表达式在编译期就能算出

// constexpr int port2 = readPortFromFile(); // 错误：普通读文件操作不能在编译期执行
```

可用下面的方式选择：

| 你的意图 | 首选写法 | 原因 |
| --- | --- | --- |
| 运行时才知道值，但之后不允许改 | `const T value = ...;` | 例如读取配置、当前时间、用户输入 |
| 值应在编译期可用 | `constexpr T value = ...;` | 可用于数组大小、模板参数、编译期计算 |
| 类的共享编译期常量 | `static constexpr T kValue = ...;` | 不属于某个对象，名称仍在类作用域内 |
| 只想限制访问者不能改对象 | `const T&`、`const T*`、const 成员函数 | 这是接口只读性，不代表编译期常量 |

例如，`std::array` 的长度属于类型的一部分，需要编译期常量：

```cpp
constexpr std::size_t kBufferSize = 1024;
std::array<char, kBufferSize> buffer{};
```

而 `std::vector` 的大小是运行时状态，所以可使用普通 `const` 值：

```cpp
const int count = readPortFromFile(); // 假设此处得到一个数量
std::vector<int> values(count);       // vector 的长度可在运行期决定
```

不要把 `constexpr` 理解为“永远没有运行时成本”。它表示**允许**在编译期求值；当上下文不能要求编译期求值时，编译器也可以在运行时执行同一个 `constexpr` 函数。

类内整数常量可写成：

```cpp
class GamePlayer {
public:
    static constexpr int kNumTurns = 5;
};
```

书中提到旧标准对某些类内常量可能还需要类外定义；在现代 C++ 中，`static constexpr` 数据成员可直接使用。若项目需兼容旧标准或遇到 ODR-use 的旧代码，再按所用标准处理定义问题。

### 2. `enum`：给有限的一组整数取有意义的名字

枚举（enumeration，简称 enum）是一种类型，用来表示“取值只应来自有限集合”的状态。例如日志级别不是任意整数，而是 debug、info、warning、error 中之一。

```cpp
// 旧式（unscoped）enum：枚举项的名字会进入外层作用域。
enum Color { red, green, blue };

Color color = red; // 可以
// int red = 1;    // 错误：red 这个名字已被枚举项占用
```

现代 C++ 默认优先使用**强类型枚举** `enum class`：

```cpp
enum class LogLevel { debug, info, warning, error };

LogLevel level = LogLevel::warning;
// LogLevel level2 = warning; // 错误：必须明确写出作用域
// int n = level;             // 错误：不会隐式转换成 int
```

`enum class` 的两个主要好处是：枚举项有自己的作用域（`LogLevel::warning`），并且不会悄悄转换为整数。它更不容易与其他名称冲突，也更能阻止把不同类别的值混在一起。需要指定底层整数类型时可写：

```cpp
enum class FilePermission : unsigned char {
    read = 1,
    write = 2,
    execute = 4,
};
```

普通业务状态、选项和错误码通常用 `enum class`；不要用裸整数加“魔法数字”表示它们。若要把它传给 C 接口、序列化或位运算，需要显式转换，并先确认接口所需的整数类型：

```cpp
auto raw = static_cast<unsigned char>(FilePermission::read);
```

#### 书中的 enum hack 是什么

书中用 enum hack 表达“编译期整数常量”：

```cpp
class GamePlayer {
private:
    enum { kNumTurns = 5 };
};
```

这里并不是在建模“多种状态”，而是在借用 enum 的“编译期整数常量”性质。它没有对象身份，不能取地址。今天优先写 `static constexpr int kNumTurns = 5;`；学习 enum hack 的价值在于读懂旧代码，并理解“编译期常量”与“运行期对象”并不相同。

### 3. 用内联函数替代函数宏

```cpp
// 错误风险：参数会被重复求值
#define CALL_WITH_MAX(a, b) ((a) > (b) ? (a) : (b))

int a = 5;
int b = 0;
CALL_WITH_MAX(++a, b); // a 可能自增两次

// 推荐：参数只按正常函数调用规则求值一次
template <class T>
constexpr const T& maxValue(const T& a, const T& b) {
    return a < b ? b : a;
}
```

注意：上面函数返回的是引用，所以传入的实参必须在使用返回值时仍然存活；对于临时对象或想要值语义的场景，可以返回 `T`，或优先使用标准库的 `std::max` 并理解其返回引用的规则。

#### `inline` 到底是什么意思

初学时很容易把 `inline` 只理解成“把函数代码直接塞到调用处，所以更快”。这只是编译器**可能**采用的一种优化，`inline` 关键字并不命令编译器一定这么做。

它更重要的语言含义是：一个 inline 函数可以在多个翻译单元中拥有**完全相同的定义**。因此小型函数常直接定义在头文件中：

```cpp
// math.h
inline int add(int left, int right) {
    return left + right;
}
```

多个 `.cpp` 包含 `math.h` 时，每个翻译单元都会看到这份函数定义；因为它是 inline，链接时这是合法的。相反，普通非 inline 函数的定义若直接放进头文件，会造成“multiple definition（重复定义）”链接错误。

以下两种情况会隐含 inline 语义：

```cpp
class Counter {
public:
    int value() const { return value_; } // 在类定义内定义的成员函数，隐含 inline
private:
    int value_{};
};

constexpr int twice(int x) { return 2 * x; } // constexpr 函数也隐含 inline
```

从 C++17 起，变量也可以是 inline，适合在头文件中定义一个跨翻译单元共享的变量：

```cpp
// settings.h
inline constexpr int kDefaultTimeoutMs = 3'000;
```

实用判断：为了在头文件中放函数定义、避免重复定义时使用 `inline`；为了表达编译期常量时使用 `constexpr`；**不要仅为了“可能更快”而到处加 `inline`**。是否展开取决于优化等级、函数大小和调用点上下文，过度展开还会让二进制变大、降低指令缓存命中率。

### 宏是否完全不能用？

不是。以下用途仍然合理或常见：

- 头文件包含保护（现代可优先用 `#pragma once`，取决于项目规范）；
- 条件编译、平台探测，例如 `#if defined(_WIN32)`；
- 少量必须依赖预处理阶段信息的构建配置。

但宏不适合作为普通常量、普通函数和作用域工具。

### 条款 2 的检查表

- 常量：能否改为带作用域的 `constexpr` 或 `const`？
- 类常量：能否改为 `static constexpr`？
- 类似函数的宏：能否改为 `constexpr`/`inline` 函数或函数模板？
- 如果必须保留宏：参数、整体表达式是否都加括号？参数是否会有副作用？

---

## 条款 3：尽可能使用 `const`

### 核心结论

`const` 是一种**接口契约**：通过某个名字、引用或指针，调用者不应修改对象。它既帮助读者理解代码，也让编译器阻止一类错误。

### `const` 首先是在限制“这条访问路径”

```cpp
int score = 80;
const int passingScore = 60;

// passingScore = 70; // 错误：const 对象初始化后不能再赋值

int& editable = score;
const int& readable = score;
editable = 90;        // 可以：通过非 const 引用修改 score
// readable = 90;     // 错误：不能通过 readable 修改
```

最后两行说明一个关键点：`const int& readable` 并没有把 `score` 变成宇宙意义上的不可变对象；它只承诺“不能**通过 `readable` 这个引用**改它”。如果对象原本不是 const，其他非 const 别名仍可修改它。

同理，`const` 默认不是深层递归的。例如 `const std::vector<User*> users` 阻止你通过 `users` 增删元素或改写指针位置，却不自动令每个 `User*` 指向的 `User` 也变成 const。需要只读用户时，应让元素类型也表达约束，例如 `std::vector<const User*>`（同时仍要明确对象所有权）。

### 先读懂 const 与指针

从右往左读最直观：

```cpp
const int* p1 = &value; // 指向 const int 的指针：不能通过 p1 改 value，p1 可以改指向
int* const p2 = &value; // const 指针：p2 不能改指向，可以通过 p2 改 value
const int* const p3 = &value; // 两者都不能改
```

`int const*` 与 `const int*` 完全等价。实际团队中选一种风格并保持一致即可。

### const 成员函数：对外承诺“观察而不修改”

```cpp
class TextBlock {
public:
    const char& operator[](std::size_t pos) const {
        return text_[pos];
    }

    char& operator[](std::size_t pos) {
        return text_[pos];
    }

private:
    std::string text_;
};
```

这叫 **const 重载**。`const TextBlock` 只能调用 const 成员函数，得到 `const char&`，因而不能修改字符；普通对象则可以修改。标准库容器的 `operator[]`、`begin()` 等也常有这类成对接口。

const 成员函数有两层含义：

- **bitwise constness（位级常量性）**：不改任何非 `mutable` 数据成员；
- **logical constness（逻辑常量性）**：从使用者可观察的状态不变。

缓存就是 logical constness 的常见例子：计算结果不改变“这个对象表示什么”，但可能写入缓存。

```cpp
class Polynomial {
public:
    double valueAt(double x) const {
        if (!cacheValid_) {
            cachedValue_ = /* 根据系数计算 */ 0.0;
            cacheValid_ = true;
        }
        return cachedValue_;
    }

private:
    mutable double cachedValue_ = 0.0;
    mutable bool cacheValid_ = false;
};
```

`mutable` 应当少用：它绕开了位级 const 检查。并发场景中，这类 const 缓存还需要同步；`const` 不等于线程安全。

### 让 const 与非 const 实现保持一致

不要复制两套几乎相同的查找逻辑。书中的做法是让非 const 版本复用 const 版本：

```cpp
class TextBlock {
public:
    const char& operator[](std::size_t pos) const { return text_[pos]; }

    char& operator[](std::size_t pos) {
        return const_cast<char&>(
            static_cast<const TextBlock&>(*this)[pos]);
    }
private:
    std::string text_;
};
```

#### 先理解：为什么会有两个 `operator[]`

`object[pos]` 是 `object.operator[](pos)` 的简写。C++ 会根据对象是否为 const 选择重载：

| 调用者的类型 | 被调用的重载 | 返回类型 | 能否通过结果修改字符 |
| --- | --- | --- | --- |
| `TextBlock` | 非 const `operator[]` | `char&` | 可以 |
| `const TextBlock` 或 `const TextBlock&` | const `operator[]` | `const char&` | 不可以 |

这使类同时提供“可读可写”和“只读”两种合理接口。真正保存字符串的逻辑只需要写在 const 版本里；非 const 版本只负责把得到的结果恢复为可写引用。

#### 把这一行从里到外拆开

```cpp
return const_cast<char&>(
    static_cast<const TextBlock&>(*this)[pos]);
```

1. `this` 是指向当前对象的隐式指针。在这个**非 const** 成员函数内，`this` 的类型可理解为 `TextBlock*`；因此 `*this` 是当前对象本身，类型为 `TextBlock&`。
2. `static_cast<const TextBlock&>(*this)` 没有创建新 `TextBlock`，也没有复制字符串。它只是明确告诉编译器：**暂时把同一个对象当成 `const TextBlock&` 看待**。
3. 对 const 引用使用 `[pos]`，重载决议就会强制选择第一版 `operator[](std::size_t) const`，其返回值为 `const char&`。
4. `const_cast<char&>(...)` 去掉该返回引用上的 const 限定，得到 `char&`，以符合非 const 重载的返回类型。

第二步为什么必须写？如果直接写 `(*this)[pos]`，当前对象是非 const，C++ 会再次选择正在实现的非 const `operator[]`，造成无限递归。把它先转换成 `const TextBlock&` 才能明确地调用另一份重载。

#### `static_cast` 与 `const_cast` 分别做什么

`static_cast<T>(expr)` 是由编译器在编译期检查的**显式类型转换**。常见用途有整数/浮点转换、相关类层次中的上行转换，以及本例中把 `TextBlock&` 视为 `const TextBlock&`。它不做运行时类型检查；尤其把基类指针/引用向下转为派生类时，若对象实际不是该派生类，不能依赖它，应考虑 `dynamic_cast`（仅适用于多态类型）。

`const_cast<T>(expr)` 的职责非常窄：只添加或去掉 `const`/`volatile` 限定，不能把 `int` 转成 `std::string`，也不能改变对象的真实类型。它本身通常不生成复制或运行时操作，只改变编译器如何看待这条访问路径。

在本例中，`const_cast` 是受控且合法的，理由是：执行非 const `operator[]` 时，原始对象必然是一个非 const `TextBlock`。我们只是先为了复用 const 逻辑而“临时只读”，再恢复它原本就拥有的可写能力。

以下写法则是危险的：

```cpp
const int original = 42;
const int& readOnly = original;
const_cast<int&>(readOnly) = 7; // 未定义行为：original 一开始就是 const
```

`const_cast` 能让代码通过编译，**不能让一个真正定义为 const 的对象变得可修改**。这类写入是未定义行为，可能表面正常、数据错误，或在只读内存页上直接崩溃。

若项目使用 C++17，还可把第二步写得更有意图：

```cpp
return const_cast<char&>(std::as_const(*this)[pos]);
```

`std::as_const(*this)` 的效果就是把对象以 const 引用形式交给后续表达式；它需要 `#include <utility>`。两种写法的核心原理相同。

### const 应放在哪里

优先从接口边界开始：

```cpp
void print(const std::string& message); // 不修改输入，也避免复制
std::string normalize(std::string value); // 按值接收，函数内部可修改副本
```

“按 `const T&` 传递总是更快”也不成立：小的可复制类型（如 `int`、小型平凡结构）通常应按值传递。条款 1 的“看具体语境”在此再次适用。

### 常见误区

- `const` 保护的是**经由当前访问路径**不能修改，不保证对象绝对不会被其他别名修改。
- 返回 `const` 的值类型在现代 C++ 中往往是坏主意：它会妨碍移动语义。例如应返回 `std::string`，而不是 `const std::string`。
- 局部变量是否写 `const` 主要是可读性选择；**函数参数、返回类型（引用/指针）和成员函数**的 const，通常更有接口价值。

---

## 条款 4：确定对象在使用前已被初始化

### 核心结论

C++ 不会自动把所有对象初始化为“安全的零值”。读取未初始化对象的值通常是未定义行为。**默认习惯应是：声明对象时立即初始化，并让每个构造函数初始化所有成员。**

### 初始化不是赋值

```cpp
class PhoneNumber {
public:
    PhoneNumber() : number_() {} // 初始化：number_ 直接构造为空字符串

private:
    std::string number_;
};
```

若写成在构造函数体内 `number_ = "";`，`number_` 会先默认构造，再进行赋值。对于类类型，这可能多一次工作；更重要的是，成员初始化列表清楚表达了对象建立时的状态。

更现代、简洁的方式是使用**类内成员初始值设定项**：

```cpp
class PhoneNumber {
private:
    std::string number_{};
    int countryCode_{86};
};
```

构造函数可只覆盖需要不同初值的成员。真正的成员初始化顺序由**成员在类中声明的顺序**决定，不是初始化列表的书写顺序；编译器警告应认真处理。

### 内置类型尤其要主动初始化

```cpp
int count;     // 局部内置变量：值不确定，不能读取
int total{};   // 值初始化为 0
double ratio{}; // 初始化为 0.0
int data[3]{}; // 全部元素为 0
```

类类型是否安全取决于其构造函数是否建立了完整不变量；不要假设“它是对象，所以一定已经正确初始化”。

### 跨翻译单元的静态对象初始化顺序问题

最难的一部分是：不同 `.cpp` 文件中的非局部静态对象，其初始化先后顺序通常不确定。

```cpp
// file_a.cpp
extern Logger globalLogger;
Config globalConfig(globalLogger); // globalLogger 可能尚未构造

// file_b.cpp
Logger globalLogger;
```

这类代码可能在某次链接方式或某个平台“碰巧能用”，换个构建环境就失败。

解决办法之一是把非局部静态对象改为返回**函数内静态对象**的访问函数。先看代码，再逐行解释：

```cpp
Logger& logger() {
    static Logger instance; // 只在第一次执行到这一行时构造一次
    return instance;        // 返回同一个 Logger 对象的别名，不复制对象
}

Config& config() {
    static Config instance{logger()}; // 假设 Config 的构造函数需要 Logger&
    return instance;                  // 后续调用也返回同一个 Config 对象
}
```

这里有三个初学者容易混淆的点：

1. `Logger&` 中的 `&` 表示**引用**。`logger()` 的返回值不是新的 `Logger` 副本，而是 `instance` 的另一个名字。因此调用 `logger().write(...)` 操作的就是唯一那份 `instance`。
2. `static Logger instance;` 定义在函数内，所以它是 **local static object（局部静态对象）**：名字 `instance` 只在 `logger()` 函数中可见，但对象本身一旦构造，会一直活到程序结束。它不像普通局部变量那样在函数返回时销毁。
3. `static Config instance{logger()};` 中的 `{logger()}` 是构造 `Config` 时传入一个 `Logger&` 参数。它要求 `Config` 有类似 `Config(Logger&)` 的构造函数。写成书中的 `instance(logger())` 含义相同；这里用 `{}` 只是更容易看出“用 `logger()` 的返回值构造 Config”。

关键变化在于：原来的 `globalLogger` 和 `globalConfig` 在 `main` 之前就自动构造；现在它们都不会在程序启动阶段构造，而是**谁第一次调用访问函数，谁触发初始化**。

假设 `main` 第一次调用的是 `config()`，实际执行顺序是：

```text
main 调用 config()
  → 第一次到达 config() 中的 static Config instance{logger()}
  → 要构造 Config，必须先计算构造参数 logger()
  → 第一次调用 logger()，构造 Logger instance
  → logger() 返回已构造好的 Logger 引用
  → 使用该引用构造 Config instance
  → config() 返回已构造好的 Config 引用
```

这条顺序完全由代码中的调用关系决定：`Config` 依赖 `Logger`，所以构造 `Config` 前必然调用 `logger()`。它不再依赖两个 `.cpp` 文件碰巧以什么顺序启动初始化。之后再调用 `config()` 或 `logger()` 时，`static` 局部对象已经存在，只会直接返回，不会再次构造。

如果先调用 `logger()`，也没有问题：它先构造 Logger；以后第一次调用 `config()` 时，`logger()` 直接返回已有的 Logger，再构造 Config。

从 C++11 起，若两个线程同时第一次调用 `logger()`，标准保证只有一个线程执行 `Logger instance` 的构造，另一个线程会等到构造完成。因此“**第一次构造**”是线程安全的；但构造完成后，两个线程同时调用 `logger().write(...)` 是否安全，仍由 `Logger::write` 自己的同步设计决定。

本例中 `Config` 一定在 `Logger` 之后构造，因此程序退出时会先析构 Config、再析构 Logger，依赖关系仍正确。更复杂的项目中若存在其他静态对象、动态库或析构期间的交叉调用，销毁顺序仍可能出问题；此时在 `main` 中显式创建并传递依赖，或用应用级生命周期管理器统一管理，通常更清晰。

### 条款 4 的检查表

- 局部内置变量是否使用 `{}` 或明确的初值？
- 每个构造函数是否初始化了所有成员，并满足类的不变量？
- 是否把“初始化”误写成了构造函数体中的“赋值”？
- 初始化列表顺序是否与成员声明顺序一致？
- 是否存在跨 `.cpp` 文件相互依赖的全局/静态对象？能否改为函数内静态对象或显式注入？

---

## 条款之间，以及与后续条款的联系

| 关联条款 | 与本章的关系 |
| --- | --- |
| 条款 5：了解 C++ 默默编写并调用哪些函数 | 构造、复制、赋值、析构都是对象初始化和资源语义的基础；学习条款 4 后应接着理解它。 |
| 条款 6：若不想使用编译器自动生成的函数，就该明确拒绝 | 当复制/赋值会破坏对象不变量或资源所有权时，需要禁止它们；现代 C++ 常写 `= delete`。 |
| 条款 7：为多态基类声明 virtual 析构函数 | 对象生命周期的另一面：经基类指针删除派生对象时必须正确析构。 |
| 条款 13：以对象管理资源 | 条款 4 保证“从一开始就是有效状态”，条款 13 进一步用 RAII 保证“离开作用域也会正确释放”。 |
| 条款 20：宁以 pass-by-reference-to-const 替换 pass-by-value | 是条款 3 在函数参数上的重要应用，但必须结合类型大小与语义判断。 |

---

## 学完后应能回答的四个问题

1. 为什么 `#define SQUARE(x) x*x` 有风险，而函数模板更可靠？
2. `const Widget*`、`Widget* const` 和 `const Widget* const` 分别限制了什么？
3. 为什么构造函数成员初始化列表通常优于在构造函数体中赋值？
4. 为什么两个 `.cpp` 文件里的全局对象不能可靠地互相依赖？

建议动手练习：把一个对象宏和一个函数宏分别改成 `constexpr` 常量、函数模板；为一个含 `std::string` 和 `int` 成员的类写两个构造函数，并用类内成员初始值与初始化列表保证两者都处于有效状态。编译时开启警告（例如 GCC/Clang 的 `-Wall -Wextra -Wpedantic -Wreorder`），把警告当作学习反馈。

---

## 现代 C++ 与工程实践补充

本书的规则不是“旧”，而是其表达方式需要放到现代工具链中理解。以下是工作中最常遇到的对应关系。

### `const`、`constexpr`、`consteval` 与 `constinit` 不同

它们都与“不可变/初始化”有关，但解决的问题不同：

| 关键字 | 核心承诺 | 是否要求编译期值 | 初始化后能否修改 | 常见用途 |
| --- | --- | --- | --- | --- |
| `const` | 不能通过该对象/访问路径写入 | 否 | 否 | 只读参数、运行期常量 |
| `constexpr` | 值或函数调用可参与编译期计算 | 在要求常量表达式的上下文中是 | 变量本身不可修改 | 尺寸、查表、纯计算 |
| `consteval` | 函数每次调用都必须在编译期完成 | 是 | 不适用 | 编译期校验、生成元数据 |
| `constinit` | 静态/线程存储期变量必须静态初始化 | 是（初始化阶段） | 可以 | 防止静态变量意外动态初始化 |

```cpp
constexpr int square(int x) { return x * x; }
constexpr int kBufferSize = square(16); // 必定为编译期常量

constinit int requestCount = 0; // 启动时已初始化为 0，但运行时可递增
```

`constinit` **不能**解决两个翻译单元之间“谁先初始化”的依赖；它只保证变量本身不是延迟到动态初始化阶段才建立。对跨文件依赖，仍应使用访问函数、显式初始化顺序或依赖注入。

对于具名整数枚举，现代代码通常使用 `enum class` 防止枚举值泄漏到外层作用域；这和条款 2 中为了常量而使用的旧式 enum hack 是两件事：

```cpp
enum class LogLevel { debug, info, warning, error };
```

### 条款 2 在大型项目中的实际含义

头文件会被许多翻译单元分别预处理。一个对象宏不仅没有类型，还会污染所有包含它的代码：

```cpp
#define MIN(a, b) ((a) < (b) ? (a) : (b))
// Windows 平台头文件中曾常见 min/max 宏，会与 std::min/std::max 发生冲突。
```

工程中优先把名称放进 `namespace`、类或匿名命名空间，并使用 `constexpr` 变量/函数模板。`inline` 变量（C++17）适合“定义在头文件、由多个翻译单元共享”的变量：

```cpp
// config.h
inline constexpr std::string_view kServiceName = "payment";
```

这里 `std::string_view` 不拥有字符串；它安全是因为字面量拥有静态存储期。若 view 指向临时 `std::string`，就会悬空。**const 表示不能改，不表示拥有，也不表示生命周期足够长。** 这是企业代码中很常见的误解。

### 条款 3 与接口、所有权和并发

当接口写成 `const T&` 时，它通常同时表达“函数不修改输入”和“避免一次复制”，但**没有**表达所有权转移或对象存活时间。工程 API 还要考虑：

- 不保存参数：`const T&` 或 `std::span<const T>` 常合适；
- 要取得所有权：按值接收，再 `std::move` 到成员中；
- 只借用连续数据：`std::span<const T>`，且调用方必须保证底层数据存活；
- 可空的借用：原始指针、`std::optional<std::reference_wrapper<const T>>`，或项目既有约定；
- 需要共享所有权：慎用 `std::shared_ptr<const T>`；它表示共享管理，不只是“只读”。

`const` 也不是线程安全承诺。一个 `const` 成员函数如果读取了会被其他线程修改的普通成员，仍可能形成数据竞争；如果通过 `mutable` 写缓存，还必须使用互斥锁、原子类型或其他同步手段。把对象发布给多个线程前，先明确“谁写、谁读、由谁同步”。

### 条款 4 与 RAII

初始化的终点不是“变量有值”，而是对象一创建就满足**不变量**：例如文件句柄要么有效打开、要么处于可识别的空状态；互斥锁封装对象要么已构造完成、要么根本不可用。

现代 C++ 将这一思路推广为 RAII（资源获取即初始化）：

```cpp
void writeLog(const std::string& path) {
    std::ofstream out(path);              // 构造时尝试打开
    if (!out) throw std::runtime_error("cannot open log file");
    out << "started\n";
} // 离开作用域自动关闭，即使中途抛出异常
```

裸 `new`/`delete`、`malloc`/`free`、`open`/`close`、`lock`/`unlock` 都容易让“获取”和“释放”分散。优先让 `std::vector`、`std::unique_ptr`、文件流、`std::lock_guard` 等对象管理它们；这正是后续条款 13 的重点。

---

## 从源码到操作系统：这些规则为什么会出现

### 1. 宏发生在编译器之前

一个典型 C++ 构建过程可粗略理解为：

```text
源文件 + 头文件
       │  预处理：展开 #include / #define / #if
       ▼
翻译单元（translation unit）
       │  编译、汇编
       ▼
目标文件（.o / .obj）
       │  链接：解析跨文件符号，生成可执行文件或库
       ▼
程序加载、运行
```

预处理器只看 token，不理解 C++ 类型、命名空间或副作用，所以函数宏会重复求值，宏名也可能撞车。`constexpr`、模板、`inline` 函数则留在编译器的类型系统中，由编译器做检查和优化。

### 2. `const` 是语言约束，不是绝对物理保护

编译器会拒绝通过 `const` 访问路径写入对象；对于某些真正的常量数据，编译器和链接器还可能把它放入只读段，操作系统加载器会把相应内存页映射为只读。此时用 `const_cast` 强行写入，除了已是 C++ 未定义行为外，在许多系统上还可能触发访问违例/段错误。

但不能反向推论“所有 const 都存于只读页”：局部 `const`、动态分配对象、通过非 const 别名仍可访问的对象，布局都可能不同。正确的依据始终是 C++ 的类型和生命周期规则，而不是某次运行恰好是否崩溃。

### 3. 为什么跨文件静态初始化没有可靠顺序

这一节最容易混淆，因为书里的 “static object（静态对象）” 说的是**存储期**，并不单纯指代码里是否写了关键字 `static`。先把三个概念分开。

#### 先区分：存储期、作用域与链接属性

| 概念 | 回答的问题 | 例子 |
| --- | --- | --- |
| 存储期（storage duration） | 对象从何时存在到何时销毁？ | 函数局部普通变量通常进入函数时创建、离开时销毁；静态存储期对象几乎贯穿整个程序。 |
| 作用域（scope） | 在源码的哪里能直接写出这个名字？ | 函数体内定义的变量只在该代码块可见。 |
| 链接属性（linkage） | 其他翻译单元能否通过同名声明指向它？ | namespace 作用域变量通常有外部链接；匿名 namespace 中的变量只有本文件可见。 |

书中所说的 static object，准确说是**具有静态存储期的对象**：它在 `main` 开始前或首次使用时初始化，并通常在程序结束时销毁。下面四种很容易被混为一谈：

```cpp
Logger globalLogger;               // 非局部静态对象：namespace/global 作用域

namespace {
Logger fileOnlyLogger;             // 仍是非局部静态对象；匿名 namespace 只改变链接属性
}

struct App {
    static Logger logger;          // 静态数据成员；定义后也是非局部静态对象
};

Logger& logger() {
    static Logger instance;        // 局部静态对象：作用域在函数内，存储期仍是静态
    return instance;
}

void work() {
    Logger temporaryLogger;        // 普通局部（automatic）对象：每次进入 work 创建，离开就销毁
}
```

因此：

- **non-local static object（非局部静态对象）**：具有静态存储期，且不定义在函数体内。常见的是全局变量、namespace 变量、静态数据成员。它们常被俗称为“全局对象”。即使没有写 `static` 关键字，`Logger globalLogger;` 也是这一类。
- **local static object（局部静态对象）**：定义在函数体/代码块内，并显式写 `static`。它的**名字只在该块中可见**，但对象第一次构造后会一直活到程序结束，不会随着函数返回销毁。
- namespace 作用域的 `static Logger x;` 中的 `static` 主要还会影响链接属性（使其仅本翻译单元可见）；它不改变 `x` 本来就具有静态存储期这一事实。

#### “翻译单元”是什么，为什么它是问题边界？

一个 `.cpp` 文件经过 `#include` 展开后得到一个翻译单元。每个 `.cpp` 分别编译成 `.o` 文件，最后由链接器把它们合并。编译器可以可靠地看到**同一个翻译单元**里变量的定义顺序，却无法从 C++ 标准得到“另一个 `.cpp` 的动态初始化应排在这里之前”的全局顺序。

例如有三个文件：

```cpp
// logger.h
class Logger {
public:
    Logger();
    void write(const char* message);
};
extern Logger globalLogger;

// logger.cpp
#include "logger.h"
Logger globalLogger; // 非局部静态对象 A

// config.cpp
#include "logger.h"
class Config {
public:
    Config() { globalLogger.write("construct Config"); }
};
Config globalConfig; // 非局部静态对象 B；构造时依赖 A
```

`globalConfig` 的构造函数必须使用已经构造完成的 `globalLogger`。但 A 和 B 位于不同翻译单元，标准没有规定它们谁先进行动态初始化：

```text
可能的顺序 1：构造 globalLogger → 构造 globalConfig     // 这次看起来正常
可能的顺序 2：构造 globalConfig → 构造 globalLogger     // B 使用了尚未开始生命周期的 A，错误
```

这就是 **static initialization order fiasco（静态初始化顺序灾难）**。它危险之处在于：同一份源码可能只因链接顺序、换编译器、加入一个动态库或开启链接时优化，就从“正常”变成故障。不能用“我机器上现在的输出”证明该依赖正确。

#### 初始化到底分几步？

具有静态存储期的对象启动时，大致经历以下阶段：

1. **零初始化**：程序装载时，静态存储区域先变为零值；在类 Unix 系统上，未显式初始化的数据常由可零填充的 `.bss` 区表示。指针此时为 null、整数为 0，但类对象的构造函数尚未执行。
2. **常量初始化**：能在编译期完成的初始化先完成，例如 `constexpr` 数据。它不依赖运行期执行构造函数。
3. **动态初始化**：需要执行构造函数或函数调用的初始化，例如 `Logger globalLogger;`。顺序问题发生在此阶段。

同一翻译单元内，非局部对象的动态初始化按**定义出现的顺序**进行；不同翻译单元之间则没有可依赖的顺序。尤其不要误以为“零初始化过，所以还没构造的 `Logger` 也能安全调用”——对象的存储有字节，不等于对象生命周期和不变量已经建立。

#### local static 为什么能解决“初始化顺序”

把对象藏在访问函数中：

```cpp
Logger& logger() {
    static Logger instance; // 第一次执行到这一行时才构造
    return instance;
}

Config& config() {
    static Config instance{logger()}; // 构造 Config 前先调用 logger()
    return instance;
}

int main() {
    config();
}
```

执行 `config()` 的过程是：

```text
首次调用 config()
  → 需要构造 Config
  → 先求值 logger()
  → 首次调用 logger()，构造 Logger
  → Logger 已就绪，构造 Config
  → 返回 Config
```

这里的依赖不再靠“程序启动时碰巧的全局顺序”，而是由函数调用关系明确表达。C++11 起，多个线程同时第一次调用 `logger()` 时，标准保证 `instance` 的初始化只会完成一次，其他线程会等待初始化完成后再继续。

#### 它没有自动解决的一切

函数内 static 是常用解法，但要知道边界：

- 它只保证**首次初始化**线程安全；之后多个线程同时读写 `Logger` 的成员，仍需要互斥锁、原子操作等同步。
- 若初始化函数抛出异常，初始化不算完成；下一次进入声明处会再次尝试初始化。
- 不要在同一线程的 local static 初始化过程中递归调用同一个访问函数；这会造成递归初始化问题，行为不应依赖。
- 程序退出时，已构造的静态对象仍会析构。如果一个静态对象析构时依赖另一个已析构对象，会出现对应的**静态销毁顺序问题**。需要复杂全局生命周期时，更清晰的方案通常是在 `main` 中显式创建对象并按依赖顺序传递，或由应用级生命周期管理器统一销毁。
- `constinit` 只能保证变量本身完成静态初始化，不能声明“先初始化 A 再初始化 B”，因此不能替代访问函数或显式依赖管理。

工程中的默认策略是：避免让可变的非局部静态对象互相依赖；能通过参数传递的依赖就显式传递。确实需要进程内唯一服务时，再考虑使用函数内 static 的访问函数，并把初始化和销毁依赖设计清楚。

### 4. `inline` 的两个层面

`inline` 的性能含义是“编译器**可以**把调用点替换为函数体”；这取决于优化等级、函数大小、调试信息、跨模块优化等，不能手工保证。语言规则层面，它还允许一个函数（或 C++17 的 inline 变量）的相同定义出现在多个翻译单元中而不违反 ODR（单一定义规则）。

因此，头文件中定义的小函数通常应标记为 `inline`，或写成类内定义/`constexpr`。但不要为了速度盲目加 `inline`：过度展开会增大二进制和指令缓存压力，反而可能变慢；用性能分析工具确认热点后再优化。

---

## 可执行实验：观察跨翻译单元静态初始化

配套代码位于 [`code/item_1to4/static_initialization`](/home/jason/study/ck_study/cplusplus/effective_c++/code/item_1to4/static_initialization)。它提供两种写法：

- `bad_*.cpp`：两个非局部静态变量跨文件依赖。交换链接时源文件顺序，在 GCC/Clang 等常见实现上通常可看到不同输出；标准不保证任何一种结果。
- `good.cpp`：将依赖对象改为函数内静态对象；无论谁先请求配置，日志设施都会先完成初始化。

### `bad_*.cpp` 到底“坏”在哪里？

这组文件故意没有直接调用一个“未构造的 Logger 对象”，而是用 `bool loggerIsReady` 做了一个**安全且可观察的替身**。真实项目里若在此处调用尚未构造对象的成员函数，往往已经是未定义行为，反而不适合作为稳定的入门实验。

| 文件 | 它做的事 | 关键点 |
| --- | --- | --- |
| `bad_logger.h` | 用 `extern` 声明两个变量 | `extern` 只告诉编译器“变量在别处定义”，不创建对象、不初始化对象。 |
| `bad_logger.cpp` | 定义 `loggerIsReady` | `bool loggerIsReady = initializeLogger();` 要调用函数，所以是动态初始化。 |
| `bad_config.cpp` | 定义 `configWasInitialized` | 它的初始化函数读取 `loggerIsReady`，即 Config 依赖 Logger 的模拟。 |
| `bad_main.cpp` | 最后打印状态 | `main` 执行前两个动态初始化都已结束，因此它只能看到“最后状态”，不代表 Config 初始化时就正确。 |

先看 `bad_logger.cpp`：

```cpp
bool loggerIsReady = initializeLogger();
```

它在程序启动时先经历零初始化，所以 `loggerIsReady` 的初始位模式是 `false`；随后才需要执行 `initializeLogger()`，打印日志并将其设为 `true`。这一步“调用函数来得到初值”就是动态初始化。

再看 `bad_config.cpp`：

```cpp
bool configWasInitialized = initializeConfig();

bool initializeConfig() {
    std::cout << loggerIsReady;
    return true;
}
```

这里 `configWasInitialized == true` 只说明 `initializeConfig()` **运行过**，完全不说明它运行时 Logger 已准备好。真正应该关心的是它读取 `loggerIsReady` 的那一刻。

当使用如下顺序链接时：

```bash
g++ bad_config.cpp bad_logger.cpp bad_main.cpp -o bad_config_first
```

在常见 GCC/Clang 实现中，通常会观察到近似以下时间线：

```text
1. 所有静态 bool 先零初始化：loggerIsReady == false
2. 运行 bad_config.cpp 的动态初始化
   → initializeConfig() 读取到 false
   → configWasInitialized 被设为 true
3. 运行 bad_logger.cpp 的动态初始化
   → initializeLogger() 执行
   → loggerIsReady 被设为 true
4. 进入 main：此时两个 bool 都已经是 true
```

因此输出会类似：

```text
[config] logger ready while config initializes: false  ← 问题发生在这里
[logger] initialize logger
[main] logger ready: true, config initialized: true    ← 这个最终状态具有迷惑性
```

如果交换源文件链接顺序，常见实现可能先初始化 logger，因而 Config 看到 `true`。但这个“修复”是假的：C++ 标准并没有承诺链接命令中哪个文件靠前，就一定先初始化哪个翻译单元的非局部对象。它只是恰好改变了当前工具链的结果。

所以 `bad` 的真正含义不是“这些文件写错了语法”，而是：**Config 在非局部静态初始化阶段需要 Logger，却没有任何标准保证 Logger 已准备好。** 这里用 bool 只会打印错误状态；若换成真实代码：

```cpp
// config.cpp，概念示例
Config globalConfig{globalLogger}; // Config 构造函数中调用 globalLogger.write(...)
```

当 `globalLogger` 尚未构造就被使用时，对象不变量还未建立，程序行为不可靠，可能崩溃也可能悄悄产生错误。

`good.cpp` 的差别是 Config 初始化时**显式调用** `logger()`；`logger()` 在返回引用前保证自己的函数内 static 已经构造。这才是从语言规则上建立依赖，而不是观察某次启动恰好顺利。

在该目录执行：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic bad_config.cpp bad_logger.cpp bad_main.cpp -o bad_config_first
./bad_config_first

g++ -std=c++17 -Wall -Wextra -Wpedantic bad_logger.cpp bad_config.cpp bad_main.cpp -o bad_logger_first
./bad_logger_first

g++ -std=c++17 -Wall -Wextra -Wpedantic good.cpp -o good
./good
```

不要把“两个 bad 程序输出一定不同”当成测试通过条件；标准恰恰允许它们相同。实验的重点是：源代码没有声明可靠依赖关系，因而不应依赖链接顺序。实际项目中应采用 `good.cpp` 的访问函数思路，或在程序入口显式构造依赖图。

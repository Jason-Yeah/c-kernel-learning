# Effective C++：条款 26～31 学习笔记

> 对应《Effective C++》（第三版）第五章 **Implementations（实现）**。
>
> 本章关心“代码已经能工作后，如何让它在异常、维护、编译和性能压力下仍然可靠”。六条建议覆盖局部变量寿命、类型转换、内部状态泄露、异常安全、inline 的真实含义，以及翻译单元的依赖边界。

---

## 本章逻辑链

```text
缩小变量存在范围（26）
       ↓
让类型转换可见且受检查（27）
       ↓
不把内部状态和生命周期暴露给外部（28）
       ↓
任意失败路径仍保持对象有效（29）
       ↓
理解优化提示与代码体积的取舍（30）
       ↓
把实现依赖隔离在翻译单元中（31）
```

## 术语速查

| 术语 | 含义 |
| --- | --- |
| 翻译单元（translation unit） | 一个 `.cpp` 文件经全部 `#include` 展开并预处理后的编译输入。 |
| ODR | One Definition Rule，单一定义规则；同一实体的定义数量和一致性受它约束。 |
| RAII | Resource Acquisition Is Initialization，资源由对象构造取得、析构释放。 |
| RVO/NRVO | 返回值优化/具名返回值优化，减少按值返回的复制。 |
| Pimpl | Pointer to Implementation，用指针隐藏实现细节。 |
| ABI | Application Binary Interface，二进制层面的对象布局、符号和调用约定。 |

---

## 条款 26：尽可能延后变量定义式的出现时间

### 核心结论

变量应在“即将需要它、且已知道正确初值”时定义。这样缩短生命周期，避免无用构造，并让变量始终处于有效初始化状态。

### 为什么不是一进入函数就把变量全声明完

```cpp
void process(int requestId) {
    AuditRecord record; // 即使 requestId 非法，也先构造
    if (requestId < 0) return;
    record = makeAuditRecord(requestId); // 默认构造后又赋值
    write(record);
}
```

问题有两个：

1. `requestId < 0` 时，`record` 从未使用却已构造和析构。
2. 对类类型，`AuditRecord record; record = ...;` 是“默认构造 + 赋值”，可能比直接构造多一次工作。

更好：

```cpp
void process(int requestId) {
    if (requestId < 0) return;
    AuditRecord record = makeAuditRecord(requestId); // 直接以正确值构造
    write(record);
}
```

变量的作用域更小，也避免后续代码意外使用一个“尚未被赋予业务含义”的对象。

### 与 RAII 和异常的关系

局部 RAII 对象从定义处开始拥有资源，到离开作用域析构：

```cpp
if (needLock) {
    std::lock_guard<std::mutex> lock(mutex);
    updateSharedData();
} // 锁只在真正需要的最小范围内持有
```

若把锁定义在函数开头，会不必要地延长临界区，降低并发；若太晚定义又可能漏保护共享数据。条款 26 不是机械地“越晚越好”，而是：**在资源首次真正需要的位置定义，并让其作用域精确覆盖使用范围。**

### 循环中的取舍

```cpp
for (const auto& request : requests) {
    if (!isValid(request)) continue;
    AuditRecord record = makeAuditRecord(request);
    write(record);
}
```

这样每轮只为有效请求构造 `record`，逻辑清楚。旧书会讨论“循环外构造一次、循环内赋值”与“每轮构造析构”的成本取舍；现代 C++ 中优先保证语义正确和最小作用域，再用性能分析决定是否值得复用对象。复用对象还可能保留容量、状态或异常后的部分修改，复杂度更高。

---

## 条款 27：尽量少做转型动作（casting）

### 核心结论

转型告诉编译器“请按我说的类型解释它”，因此可能绕过类型系统的保护。优先改接口、使用多态或模板；确实需要时，使用语义明确的 C++ 风格 cast，避免 C 风格强转。

### 四种主要 C++ cast

| 写法 | 用途 | 关键风险 |
| --- | --- | --- |
| `static_cast<T>(x)` | 数值转换、明确的编译期类型转换、已确认安全的层次转换 | 向下转型不做运行时检查，类型猜错会出问题。 |
| `dynamic_cast<T>(x)` | 多态基类间的安全向下/横向转型 | 有运行时检查；失败时指针为 `nullptr`，引用抛 `std::bad_cast`。 |
| `const_cast<T>(x)` | 添加/去除 `const` / `volatile` | 修改原本定义为 const 的对象是未定义行为。 |
| `reinterpret_cast<T>(x)` | 低层位模式重新解释 | 高度平台相关，易违反对齐、别名和生命周期规则。 |

```cpp
Animal& animal = getAnimal();
if (auto* dog = dynamic_cast<Dog*>(&animal)) {
    dog->fetch(); // 只有真实对象确为 Dog 才执行
}
```

`dynamic_cast` 需要基类是多态类型（通常至少有一个 virtual 函数）。若你经常向下转型，往往说明基类接口缺少应有的 virtual 行为；优先把行为放到多态接口，而不是让调用者猜真实派生类型。

### 为什么 C 风格转型更糟

```cpp
int value = 7;
double ratio = (double)value / 2;              // C 风格：意图不清
double better = static_cast<double>(value) / 2; // C++ 风格：明确数值转换
```

C 风格 cast 可能在背后尝试多种转换组合，读代码时看不出它是在去 const、做层次转换还是低层重解释；搜索工具也难以准确定位。C++ 风格 cast 把风险类别写在语法中。

### 转型与性能、底层规则

`dynamic_cast` 的实现通常依赖 RTTI（运行时类型信息），对象的虚函数表/类型信息可帮助判断真实派生类型；具体布局属于 ABI 实现细节。`static_cast` 不做这类检查，因此通常更轻，但“轻”不等于可以猜测类型。

`reinterpret_cast` 常被误以为“只是改指针类型、不产生机器指令，所以安全”。即使没有指令，编译器的优化仍依据严格别名、对齐和对象生命周期规则；错误重解释可能在优化级别变化后才暴露。日常业务代码应极少使用它。

---

## 条款 28：避免返回指向对象内部成分的 handles

### 核心结论

不要轻易返回指向内部数据的引用、指针、迭代器、`string_view`、`span` 等“handle”。它们会把内部表示和生命周期暴露给调用者，并可能在对象修改、移动、销毁或容器扩容后悬空。

### 什么是 handle

handle 是让调用者间接接触对象内部成分的东西：

```cpp
const std::string& name() const; // 引用
const Point* origin() const;     // 指针
std::vector<int>::iterator begin(); // 迭代器
std::string_view prefix() const; // 非拥有视图
std::span<const int> values() const; // 连续数据视图
```

它们常避免复制，但调用者获得了“内部对象仍然存在且地址/布局不变”的隐含前提。

### const 不能解决生命周期问题

```cpp
class Playlist {
public:
    const std::string& titleAt(std::size_t index) const { return titles_.at(index); }
    void add(std::string title) { titles_.push_back(std::move(title)); }
private:
    std::vector<std::string> titles_;
};

const std::string& borrowed = playlist.titleAt(0);
playlist.add("new song"); // vector 可能重新分配，borrowed 可能悬空
```

`const` 只限制“不能通过 borrowed 修改字符串”；它不保证 `Playlist` 继续存在，也不阻止 `vector` 扩容使引用失效。

### 选择返回值、引用还是 view

| 需求 | 常见选择 | 代价/责任 |
| --- | --- | --- |
| 调用者需要独立长期保存 | 按值返回 `std::string`、`std::vector<T>` | 有复制/移动成本，但无悬空依赖 |
| 调用者只在 owner 存活且不修改时短暂读取 | `const T&`、`std::span<const T>`、`string_view` | 调用者必须遵守生命周期与失效规则 |
| 调用者需要修改内部状态 | 受控成员函数，而非裸非 const 引用 | 类可维护不变量 |

现代 C++ 中 `string_view`/`span` 很方便，但它们不拥有数据；把它们存到异步回调、成员变量或跨线程队列中前，必须先确认 owner 的寿命。

### 更深一层：封装和别名

返回 handle 会增加“谁可能持有这个内部对象别名”的数量。类以后不能自由更换 `vector` 为链表、懒加载字符串、加锁缓存或 Pimpl 表示，因为外部代码可能依赖地址稳定性。条款 22 的 private 数据与本条共同构成封装边界：private 阻止直接访问，本条避免通过 getter 又把内部地址泄露出去。

---

## 条款 29：为“异常安全”而努力是值得的

### 核心结论

异常安全不是“函数绝不抛异常”，而是“即使异常发生，也不泄漏资源、不破坏不变量，并尽可能维持调用前状态”。

### 三个保证等级

| 等级 | 异常发生后 | 例子 |
| --- | --- | --- |
| 不抛异常保证（no-throw） | 保证不抛；操作总成功或内部处理失败 | `swap`、析构、部分 `clear` |
| 强保证（strong） | 成功则完全完成；失败则对象保持原状 | copy-and-swap、先构造新状态再提交 |
| 基本保证（basic） | 对象仍有效、无资源泄漏，但值可能改变 | 很多容器操作的常见保证 |

### 错误模式：先破坏旧状态，再执行可能抛异常的工作

```cpp
void Profile::updateUnsafe(std::string name, std::vector<int> scores) {
    name_ = std::move(name);       // 已改变对象
    scores_ = std::move(scores);   // 若此处或中间操作抛异常，状态可能只更新一半
}
```

正确思路是“先在临时对象中完成所有可能失败的工作，再一次性提交”：

```cpp
void Profile::updateStrong(std::string name, std::vector<int> scores) {
    Profile next{std::move(name), std::move(scores)}; // 构造失败时 *this 未变
    swap(*this, next);                               // 不抛异常地提交新状态
}
```

这就是事务式思维：准备阶段可失败；提交阶段应小且不抛异常；失败时销毁临时状态，旧对象自动保持。

### RAII 是异常安全的地基

```cpp
auto file = std::make_unique<File>(path);
parse(*file); // 若 parse 抛异常，file 在栈展开时析构
```

RAII 自动解决“资源泄漏”，但不自动保证“业务状态没有被部分修改”。例如同时更新两个成员、两个容器或两个外部系统时，仍要设计提交顺序、回滚或事务。

### 异常安全的边界

强保证通常依赖 `swap` 不抛异常，并可能需要额外临时内存。系统资源耗尽、线程取消、外部数据库写入等场景未必能轻易做到强保证；此时至少提供基本保证，并在接口文档中说明异常后对象状态。

不要在析构函数中让异常逃出（条款 8）；栈展开期间第二个异常会导致 `std::terminate()`。

---

## 条款 30：透彻了解 inlining 的里里外外

### 核心结论

`inline` 有两层含义：语言层面允许同一函数定义出现在多个翻译单元；优化层面编译器**可能**把调用替换为函数体。它不是“强制更快”的命令。

### 语言规则层面：避免 ODR 重复定义错误

```cpp
// math.h
inline int square(int value) { return value * value; }
```

多个 `.cpp` 包含这个头文件时，每个翻译单元都会看见函数定义；因 `inline`，只要定义一致，这是合法的。类定义内部定义的成员函数、`constexpr` 函数也隐含 inline 语义。

### 优化层面：编译器自己决定是否展开

```cpp
inline int square(int value) { return value * value; }
```

编译器可能将 `square(x)` 变为 `x * x`，省去一次 call/return；也可能因调试模式、递归、函数过大、未见定义、优化策略或代码体积限制而不展开。反过来，即使未写 `inline`，优化器也可能内联一个小函数。

### 为什么过度内联可能更慢

内联会复制函数机器码到多个调用点：

```text
优点：省调用开销；常量传播、死代码消除机会更多
代价：二进制变大；指令缓存（I-cache）命中率下降；编译更慢；调试栈更难读
```

现代 CPU 执行速度常受缓存和分支预测影响；把冷门大函数强行复制到许多热点路径，可能比一次普通调用更差。性能决定应基于 profiling，而不是关键字直觉。

### inline 与 ABI/库发布

inline 函数体通常在头文件中分发，调用者编译时可能把旧函数体嵌入自己的二进制。库作者以后修改 inline 实现，已编译的客户端未必自动获得新逻辑；对稳定二进制库接口，慎把复杂行为放进 public inline 函数。

模板通常必须在头文件可见以供实例化，因此常与 inline/header-only 设计相关；这会增加编译依赖，正是条款 31 要控制的问题。

---

## 条款 31：将文件间的编译依赖关系降至最低

### 核心结论

头文件中只暴露调用者完成编译所需的声明，不暴露私有实现类型和不必要的头文件。这样实现变化不会触发大规模重编译，也降低 ABI 和耦合风险。

### 为什么 include 会放大改动

```text
app.cpp includes widget.h
widget.h includes vector, database_client, image_decoder...
任何一个私有实现头文件变化
→ widget.h 视为变化
→ app.cpp 等所有包含者都要重新编译
```

`#include` 是文本包含，不是“链接到已编译模块”。编译器会把头文件内容复制到每个翻译单元；因此公共头文件依赖越多，增量构建越慢。

### 前向声明：只在指针/引用场景延迟依赖

```cpp
class Database; // 前向声明：只告诉编译器它是一个类型

class ReportService {
public:
    void generate(const Database& database); // 引用大小已知，不需完整 Database
};
```

但不能在头文件中按值保存未完成类型：

```cpp
class Database;
class Bad {
    // Database database_; // 错误：成员大小未知，必须 include 完整定义
    Database* database_{};  // 可以：指针大小已知
};
```

### Pimpl：把按值的大型实现也隐藏起来

```cpp
// widget.h
class Widget {
public:
    Widget(std::string name);
    ~Widget(); // 在 widget.cpp 定义
    std::string summary() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

`widget.h` 只需要公开 `Widget` 的接口和一个指针；`Impl` 的 `vector`、缓存、数据库客户端等复杂成员可放到 `widget.cpp`。条款 25 中 Pimpl 能高效 swap，这里它还能隔离编译依赖。

### 现代 C++ 的补充

- 使用 include guard 或 `#pragma once` 防止重复包含；它们防循环/重复定义，但不会减少依赖量。
- 优先在头文件包含“自己接口真正需要的标准库头”，不要依赖其他头间接包含。
- C++20 modules 能减少文本包含的部分编译成本和宏污染，但模块接口仍应保持最小依赖；它不是自动解除设计耦合的魔法。
- 不要在头文件滥用 `using namespace ...;`，尤其避免 `using namespace std;`，否则会污染每个包含者的名字查找。

### Pimpl 的代价

Pimpl 通常额外一次堆分配、一次指针间接访问，并使复制、移动、析构的实现更复杂。它适合稳定库接口、编译依赖重、实现变化频繁或私有类型昂贵的类；小型内部值类型不必机械使用。

---

## 条款 26～31 的检查清单

1. 变量是否在真正需要前就构造？其作用域是否比资源使用范围更大？
2. 某个 cast 能否由更好的 virtual 接口、模板、重载或数据模型取代？
3. getter 返回的引用/指针/view 在 owner 改动或销毁后是否会悬空？
4. 任何可能抛异常的操作后，对象是否仍满足不变量？目标保证等级是什么？
5. `inline` 是为 ODR 还是为了实测热点？代码膨胀和 ABI 影响是否可接受？
6. 公共头文件是否暴露了私有实现头、按值成员或不必要的模板依赖？能否前向声明/Pimpl？

---

## 配套可执行示例

代码位于 [`code/item_26to31`](/home/jason/study/ck_study/cplusplus/effective_c++/code/item_26to31)。普通示例可独立编译：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic item26_late_definition.cpp -o item26
./item26
```

条款 31 是多文件示例，编译命令为：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic item31_widget.cpp item31_main.cpp -o item31
./item31
```

| 条款 | 文件 | 观察重点 |
| --- | --- | --- |
| 26 | `item26_late_definition.cpp` | 只有满足前置条件才构造局部 RAII 对象。 |
| 27 | `item27_minimize_casts.cpp` | `dynamic_cast` 的检查与 `static_cast` 数值转换。 |
| 28 | `item28_internal_handles.cpp` | 引用在 `vector` 可能扩容后为何不应继续使用。 |
| 29 | `item29_exception_safety.cpp` | 先准备临时状态、再 swap 提供强保证。 |
| 30 | `item30_inline.cpp` | inline 是 ODR 规则，不保证机器码一定内联。 |
| 31 | `item31_widget.h/.cpp/main.cpp` | 前向声明与 Pimpl 隔离实现依赖。 |

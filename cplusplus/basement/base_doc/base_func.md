# C++ 可调用对象（Callable Objects）深入剖析

## 概述

C++ 提供了多种方式来表示和操作**可调用对象**（Callable Objects）——即能够被 `()` 运算符调用的实体。包括传统的**函数指针**、**仿函数（Functors）**、**Lambda 表达式**、**`std::function`** 和 **`std::bind`** 等。这些工具极大地增强了 C++ 的灵活性和表达能力，尤其在处理回调、事件驱动编程和函数式编程时表现尤为出色。

理解这些机制的**内存布局**和**底层实现**，是写出高性能、可维护代码的关键。

---

## 1. 函数指针

### 1.1 基本概念

函数指针是指向**代码段（Text Segment）**中函数入口地址的指针变量。它是最原始、最轻量级的可调用对象——本质上就是一个 **8 字节（64 位）的地址值**。

```
内存布局示意：
                ┌──────────────────────┐
 函数指针变量    │  0x4012a0 (地址值)    │  →  位于栈/数据段，8字节
                └──────────┬───────────┘
                           │ 指向
                           ▼
                ┌──────────────────────┐
 目标函数体      │  push rbp            │  →  位于代码段（.text）
                │  mov rbp, rsp        │     只读，不可修改
                │  ...                 │
                └──────────────────────┘
```

### 1.2 普通函数指针

```cpp
#include <iostream>

// ─── 普通函数 ───
int add(int a, int b) { return a + b; }

// ─── 函数指针类型别名 ───
using FuncPtr = int(*)(int, int);

// ─── 接受函数指针作为参数 ───
int invoke(int x, int y, FuncPtr func) {
    return func(x, y);
}

int main() {
    // 方式 1：直接赋值
    int (*fp)(int, int) = add;
    std::cout << fp(3, 4) << '\n';           // 7

    // 方式 2：取地址符（等价）
    fp = &add;
    std::cout << fp(3, 4) << '\n';           // 7

    // 方式 3：通过类型别名
    FuncPtr fp2 = add;
    std::cout << invoke(5, 6, fp2) << '\n';  // 11

    // 方式 4：直接传函数名（隐式转换）
    std::cout << invoke(7, 8, add) << '\n';  // 15

    return 0;
}
```

### 1.3 静态成员函数指针

静态成员函数**没有 `this` 指针**，本质上与普通函数无异，可用普通函数指针指向。

```cpp
class Calculator {
public:
    static int multiply(int a, int b) { return a * b; }
    static int divide(int a, int b)   { return a / b; }
};

void demo_static_func_ptr() {
    // 声明静态成员函数指针：返回值+参数列表与普通函数指针完全一致
    int (*sfp)(int, int) = &Calculator::multiply;
    std::cout << sfp(6, 7) << '\n';      // 42
}
```

### 1.4 非静态成员函数指针

非静态成员函数有一个隐形的 `this` 参数，其指针语法特殊：

```cpp
class MyClass {
public:
    int getValue() const { return 42; }
    void setValue(int v) { value_ = v; }
private:
    int value_ = 0;
};

void demo_member_func_ptr() {
    // ─── 声明成员函数指针 ───
    int (MyClass::*mfp)() const = &MyClass::getValue;
    //         ^^^^^^^^ 注意作用域 ::

    MyClass obj;
    // 通过对象调用
    std::cout << (obj.*mfp)() << '\n';    // 42  ← .* 运算符

    MyClass* p = &obj;
    // 通过指针调用
    std::cout << (p->*mfp)() << '\n';     // 42  ← ->* 运算符
}
```

### 1.5 成员函数指针的内存布局（Itanium C++ ABI）

非虚成员函数指针底层就是一个普通地址。但**虚函数**的成员函数指针要复杂得多——Itanium ABI 下用 16 字节结构体表示：

```
struct MemberFunctionPointer {
    ptrdiff_t ptr;        // 8字节：函数地址（或 vtable index + 1）
    ptrdiff_t adj;        // 8字节：this 调整量（用于多重继承）
};
```

```cpp
class Base   { virtual void f(); int a; };
class Middle { virtual void g(); int b; };
class Derived : public Base, public Middle {
    virtual void f() override;
    virtual void g() override;
};

// Derived::g 的成员函数指针：
//   ptr = vtable_index_of_g_in_Middle + 1  （奇数值标记虚函数）
//   adj = sizeof(Base)                      （this 从 Derived* → Middle* 的偏移）
```

| 编译器 | 大小 | 备注 |
|--------|------|------|
| GCC (Itanium) | 16 字节 | `{ptr, adj}` 双字结构 |
| MSVC (Windows) | 8 字节 | 使用 `thunk` 技术，单指针搞定 |
| Clang (Itanium) | 16 字节 | 同 GCC |

---

## 2. 仿函数（Functors）

### 2.1 概念

仿函数是**重载了 `operator()` 的类/结构体对象**。由于它可以像函数一样被调用，同时又能拥有成员变量保存状态，因此比函数指针更强大。

假设有一个 `ThresholdFilter` 类（见 [2.3 节](#23-带参数的仿函数)）：

```cpp
struct ThresholdFilter {
    int threshold_;        // 成员变量——存在 对象的内存 中
    bool operator()(int value) const {
        return value > threshold_;   // 函数体——存在 代码段 中
    }
};

ThresholdFilter filter(20);  // 在栈上创建对象
```

这个 `filter` 对象在内存中实际是**分处两个不同区域**的：

```
┌─────────────────────────────────────────────────────────┐
│                    进程虚拟地址空间                       │
│                                                         │
│  高地址                                                  │
│  ┌────────────────────────────────────────┐             │
│  │  栈（Stack） ← 这里是 filter 对象本体   │             │
│  │                                        │             │
│  │  filter 对象占用 4 字节：               │             │
│  │  ┌──────────────────┐                  │             │
│  │  │ threshold_ = 20  │  ← 成员变量      │             │
│  │  └──────────────────┘                  │             │
│  │  地址: 0x7ffd....1000                  │             │
│  │                                        │             │
│  │  注意：对象里只有成员变量，              │             │
│  │  没有函数代码！                         │             │
│  └────────────────────────────────────────┘             │
│                                                         │
│  ┌────────────────────────────────────────┐             │
│  │  代码段（.text）← operator() 的指令在这  │             │
│  │                                        │             │
│  │  ThresholdFilter::operator() const:    │             │
│  │  地址: 0x004012a0                      │             │
│  │  ┌────────────────────────────────┐   │             │
│  │  │ mov    eax, [rdi]    ; 取 threshold_│  ← rdi 指向  │
│  │  │ cmp    eax, esi      ; 比较 value   │    栈上的    │
│  │  │ setg   al            ; 返回 bool    │    filter    │
│  │  │ ret                                 │     对象     │
│  │  └────────────────────────────────┘   │             │
│  │                                        │             │
│  │  关键：代码只有一份，所有 ThresholdFilter │             │
│  │  实例共用同一段 operator() 指令。        │             │
│  │  哪个实例调用的？靠 this 指针（rdi）区分！│             │
│  └────────────────────────────────────────┘             │
│  低地址                                                  │
└─────────────────────────────────────────────────────────┘

       filter(20)              filter(100)        两个不同实例
    ┌──────────────┐        ┌──────────────┐      各自独立占用
    │ threshold_=20 │        │threshold_=100│      栈空间
    └──────────────┘        └──────────────┘
           │                       │
           │ 共用同一份代码         │
           ▼                       ▼
    ┌──────────────────────────────────────┐
    │  0x004012a0: ThresholdFilter::       │
    │  operator() const 的机器指令         │  ← 代码段，只读
    └──────────────────────────────────────┘
```

所以「仿函数」这个名字很形象：它**是个对象（数据在栈上），但能像函数一样被调用（跳转去代码段执行）**。

#### 仿函数的主要优点

| 优点 | 说明 | 对比函数指针 |
|------|------|-------------|
| **可携带状态** | 成员变量保存调用间的上下文，每次调用可改变状态 | 函数指针无状态，只能用全局/静态变量，线程不安全 |
| **可内联（零开销）** | 编译器在编译期知道具体类型，`operator()` 可直接内联展开 | 函数指针是间接调用（`call [rax]`），编译器无法内联 |
| **可参数化构造** | 构造函数传入参数，同一个仿函数类可生成行为不同的实例 | 函数指针需要不同函数名或 if-else 分支 |
| **支持访问控制** | 状态可设为 `private`，对外只暴露 `operator()` 接口 | 全局函数无封装 |
| **可与 STL 算法无缝配合** | `std::for_each`、`std::sort`、`std::find_if` 等直接接受仿函数对象 | 也可用，但无状态限制使其适用场景更窄 |

```cpp
// 函数指针做不到的事：带状态的算法
struct RunningAverage {
    double sum = 0;
    int count = 0;
    void operator()(double val) {
        sum += val;
        ++count;
    }
    double result() const { return count ? sum / count : 0; }
};

// std::for_each 返回仿函数，调用结束后状态仍在
std::vector<double> data = {1.5, 2.3, 3.7, 4.1};
RunningAverage avg = std::for_each(data.begin(), data.end(), RunningAverage{});
std::cout << avg.result();  // 2.9
```

### 2.2 基本示例

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// ─── 有状态的仿函数 ───
class EvenCounter {
public:
    EvenCounter() : count_(0) {}

    // 重载 () 运算符
    void operator()(int value) {
        if (value % 2 == 0) {
            std::cout << "偶数: " << value << '\n';
            ++count_;
        }
    }

    int count() const { return count_; }

private:
    int count_;  // 状态：记录遇见的偶数个数
};

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6};

    // 仿函数可以保存调用过程中的状态
    EvenCounter counter = std::for_each(nums.begin(), nums.end(), EvenCounter());
    std::cout << "共发现 " << counter.count() << " 个偶数\n";

    return 0;
}
```

### 2.3 带参数的仿函数

```cpp
class ThresholdFilter {
public:
    explicit ThresholdFilter(int threshold) : threshold_(threshold) {}

    bool operator()(int value) const {
        return value > threshold_;
    }

private:
    int threshold_;
};

void demo_functor_with_params() {
    std::vector<int> data = {10, 25, 3, 47, 18, 99};

    // 创建阈值为 20 的仿函数实例
    ThresholdFilter filter(20);

    // 配合标准库算法使用
    auto it = std::find_if(data.begin(), data.end(), filter);
    if (it != data.end()) {
        std::cout << "第一个 >20 的数是: " << *it << '\n';  // 25
    }

    // 可以轻松创建不同阈值的实例
    auto count = std::count_if(data.begin(), data.end(), ThresholdFilter(30));
    std::cout << "大于30的数有 " << count << " 个\n";        // 2 (47, 99)
}
```

### 2.4 仿函数的性能优势 vs 函数指针

仿函数通常比函数指针**更容易内联（inline）**，因为编译器在编译期就知道具体调用哪个 `operator()`，而对于函数指针，编译器通常只能做间接调用（通过地址跳转）。

```cpp
// ─── 函数指针版本 ───
bool greater_than_20(int x) { return x > 20; }
// 调用: std::find_if(begin, end, greater_than_20);
// 汇编: call [rax]  ← 间接调用，难以内联

// ─── 仿函数版本 ───
struct Gt20 {
    bool operator()(int x) const { return x > 20; }
};
// 调用: std::find_if(begin, end, Gt20{});
// 汇编: jmp  <地址>  ← 直接调用，容易内联
```

### 2.5 内存分布对比

用同一个 `ThresholdFilter` 示例对比**仿函数对象** vs **函数指针**在内存中的实际模样：

```
┌────────────────────────────────────────────────────────────────┐
│                    仿函数对象（栈上）                            │
│                                                                │
│  ── 对象本体在栈上，只存数据 ──                                  │
│  filter (地址 0x7ffd00001000)                                   │
│  ┌──────────────────────────┐                                  │
│  │ threshold_ = 20  (偏移+0)│  4 字节 (int)                    │
│  │ padding                  │  4 字节 (对齐到 8 字节边界)       │
│  │ [无 vptr]                │  没有虚函数 → 没有虚表指针        │
│  └──────────────────────────┘  对象共 8 字节                    │
│         │                                                      │
│         │ operator() 不在对象里，在代码段：                     │
│         ▼                                                      │
│  ── 函数代码在代码段，所有实例共用 ──                             │
│  代码段地址 0x004012a0:                                         │
│  ┌────────────────────────────────────────┐                    │
│  │ ThresholdFilter::operator() const:     │                    │
│  │    mov    eax, [rdi]    ; rdi = this   │                    │
│  │    cmp    eax, esi      ;               │                    │
│  │    setg   al                            │                    │
│  │    ret                                  │                    │
│  └────────────────────────────────────────┘                    │
│                                                                │
│  关键点：                                                      │
│  • sizeof(filter) = 8 — 对象大小只算成员变量                    │
│  • operator() 指令在代码段，sizeof 不包含它                     │
│  • 两个 filter(20) 和 filter(100) 共用同一套指令                │
│  • this (rdi) 指向不同的栈地址，区分操作哪个实例                 │
├────────────────────────────────────────────────────────────────┤
│                    函数指针（栈上）                             │
│                                                                │
│  ── 变量本身在栈上 ──                                           │
│  fp (地址 0x7ffd00000ff8)                                      │
│  ┌──────────────────────────┐                                  │
│  │ 0x004012f0               │  8 字节 = 一个地址值              │
│  └──────────────────────────┘                                  │
│         │                                                      │
│         ▼ 指向                                                 │
│  ── 目标函数在代码段 ──                                          │
│  代码段地址 0x004012f0:                                         │
│  ┌────────────────────────────────────────┐                    │
│  │ greater_than_20(int):                  │                    │
│  │    cmp    esi, 20                       │                    │
│  │    setg   al                            │                    │
│  │    ret                                  │                    │
│  └────────────────────────────────────────┘                    │
│                                                                │
│  关键点：                                                      │
│  • sizeof(fp) = 8 — 就是一个裸地址                             │
│  • 函数指针不带状态，也无法携带状态                              │
│  • 调用时必须通过地址间接跳转 (call [rax])                       │
└────────────────────────────────────────────────────────────────┘
```

总结差异：

| 对比项 | 仿函数对象 | 函数指针 |
|--------|-----------|---------|
| 栈上存放 | 成员变量（如 threshold_） | 一个地址值 |
| sizeof | 成员变量大小 + padding | 8 字节（64位） |
| 能否携带状态 | ✅ 可以（存在成员变量里） | ❌ 不能 |
| 调用方式 | 直接调用（可内联） | 间接调用（call [rax]） |
| 代码在 | 代码段，所有实例共用 | 代码段，函数指针指向它 |
| 识别哪个实例 | 靠 this 指针（rdi） | 无此概念 |


---

## 3. Lambda 表达式

### 3.1 概念

Lambda 表达式是 C++11 引入的**匿名仿函数**的语法糖。编译器会将每个 lambda 表达式展开为一个**匿名的仿函数类**（closure type），并生成该类的实例（closure object）。

基本用法：

```cpp
[captures](parameters) -> return_type {
    // the body of function
}
```

```cpp
编译器视角：你的 Lambda
    [capture](params) -> ret { body }
                ↓
编译器生成的匿名仿函数类：
    class __lambda_XXXX {
    private:
        捕获的变量们;           // 按值/按引用
    public:
        auto operator()(params) const -> ret { body }
        // 如果是 mutable 的 lambda，operator() 非 const
    };
```

### 3.2 基本语法与捕获

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6};
    int threshold = 3;

    // ─── 最简单的 lambda ───
    auto print = [](int x) { std::cout << x << ' '; };
    std::for_each(nums.begin(), nums.end(), print);
    std::cout << '\n';

    // ─── 按值捕获 ───
    auto is_above = [threshold](int x) { return x > threshold; };
    auto count = std::count_if(nums.begin(), nums.end(), is_above);
    std::cout << "大于" << threshold << "的数有 " << count << " 个\n";

    // ─── 按引用捕获（可修改外部变量） ───
    int sum = 0;
    std::for_each(nums.begin(), nums.end(), [&sum](int x) { sum += x; });
    std::cout << "总和 = " << sum << '\n';

    // ─── 捕获所有外部变量 ───
    int factor = 2, offset = 1;
    auto transform = [=](int x) { return x * factor + offset; };  // [=] 按值捕获全部
    auto transform2 = [&](int x) { return x * factor + offset; }; // [&] 按引用捕获全部

    // ─── 混合捕获 ───
    auto mixed = [factor, &offset](int x) { return x * factor + offset; };

    return 0;
}
```

### 3.3 Lambda 的内存分布 —— 关键理解

Lambda 对象的大小**完全取决于捕获了什么**。这是一个经常被误解的点：

```cpp
#include <iostream>

int main() {
    // ─── 无捕获 lambda：等价于一个空结构体 ───
    auto lambda0 = []() { return 42; };
    std::cout << "无捕获 lambda 大小: " << sizeof(lambda0) << '\n';
    // 输出: 1  ← 空类，占 1 字节（C++ 要求每个对象有唯一地址）

    // ─── 按值捕获一个 int ───
    int x = 100;
    auto lambda1 = [x]() { return x; };
    std::cout << "捕获一个 int: " << sizeof(lambda1) << '\n';
    // 输出: 4  ← 内部存储了一个 int

    // ─── 按值捕获一个 double ───
    double y = 3.14;
    auto lambda2 = [y]() { return y; };
    std::cout << "捕获一个 double: " << sizeof(lambda2) << '\n';
    // 输出: 8

    // ─── 按值捕获多个变量 ───
    int a = 1; double b = 2.0; char c = 'A';
    auto lambda3 = [a, b, c]() { return a + b + c; };
    std::cout << "捕获 int+double+char: " << sizeof(lambda3) << '\n';
    // 输出: 24  ← 8(对齐double) + 8(double) + 4(int) + 1(char) + 3(padding)

    // ─── 按引用捕获：只存指针 ───
    auto lambda4 = [&x, &y]() { return x + y; };
    std::cout << "按引用捕获两个: " << sizeof(lambda4) << '\n';
    // 输出: 16  ← 两个指针，各 8 字节

    // ─── 混合捕获 ───
    auto lambda5 = [x, &y]() { return x + y; };
    std::cout << "混合捕获(值int+引用double): " << sizeof(lambda5) << '\n';
    // 输出: 16  ← 4(int) + 4(padding) + 8(指针)
    //            或者 8(指针) + 4(int) + 4(padding)（取决于成员顺序）

    return 0;
}
```

### 3.4 lambda 的内存布局可视化

```
无捕获 lambda:          捕获一个 int:           捕获 int+double+char:
┌──────┐               ┌──────┐                ┌───────────────┐
│ 空   │ 1 字节        │  x   │ 4 字节          │  b (double)   │ 8 字节  ← 对齐要求
└──────┘               └──────┘                ├───────────────┤
                                               │  a (int)      │ 4 字节
                                               │  c (char)     │ 1 字节
                                               │  padding      │ 3 字节
                                               └───────────────┘ 共 24 字节

按引用捕获两个变量:
┌───────────────────────┐
│ ptr to x (8字节)      │  ← 指向栈上的 x
├───────────────────────┤
│ ptr to y (8字节)      │  ← 指向栈上的 y
└───────────────────────┘ 共 16 字节
```

### 3.5 `mutable` lambda

`mutable` 是 C++ 中用来**在 const 语境下解除特定成员修改限制**的关键字，有两种用法：

**① `mutable` 成员变量**（C++98）：标记成员变量即使在 `const` 成员函数中也能被修改。

```cpp
class Cache {
    mutable int hit_count_ = 0;  // 即使 const 方法也能修改
    std::string compute(int key) const {
        ++hit_count_;            // ✅ 可以，因为 mutable
        return /* 耗时计算结果 */;
    }
};
```

常见用途：mutex（`mutable std::mutex m_;` const 函数中也要能加锁）、引用计数、缓存、调试计数器。

**② `mutable` lambda**（C++11）：默认 lambda 的 `operator()` 是 `const` 的——不能修改按值捕获的变量。加 `mutable` 可解除限制：

```cpp
void demo_mutable_lambda() {
    int count = 0;

    // 按值捕获，mutable 允许修改副本（不影响原变量）
    auto counter = [count]() mutable {
        return ++count;  // 修改的是 lambda 内部拷贝的 count
    };

    std::cout << counter() << '\n';  // 1
    std::cout << counter() << '\n';  // 2
    std::cout << "原 count: " << count << '\n';  // 0 ← 外部不受影响
}
```

### 3.6 泛型 lambda（C++14）

```cpp
void demo_generic_lambda() {
    // C++14: auto 参数
    auto add = [](auto a, auto b) { return a + b; };

    std::cout << add(3, 4)       << '\n';   // 7   (int)
    std::cout << add(3.14, 2.86) << '\n';   // 6.0 (double)
    std::cout << add(std::string("hello "), "world") << '\n'; // hello world
}
```

### 3.7 Lambda 与函数指针的隐式转换

**无捕获 lambda** 可以隐式转换为函数指针：

```cpp
void demo_lambda_to_fptr() {
    // 无捕获 lambda → 函数指针
    int (*fptr)(int, int) = [](int a, int b) { return a + b; };
    std::cout << fptr(10, 20) << '\n';  // 30

    // 常用于 C API 回调
    // void qsort(void* base, size_t num, size_t size,
    //            int (*compar)(const void*, const void*));

    int arr[] = {3, 1, 4, 1, 5, 9};
    std::qsort(arr, 6, sizeof(int),
               [](const void* a, const void* b) -> int {
                   return *(int*)a - *(int*)b;
               });
}
```

有捕获的 lambda **不能**转为函数指针，因为它的调用需要访问存储在对象中的捕获变量。

### 3.8 Lambda + `shared_ptr`：跨线程生命周期安全

这是 C++ 并发编程中的一个经典模式——用 **lambda 按值捕获 `shared_ptr`** 来保证对象在另一个线程中执行时不会被销毁。

#### 问题场景

```cpp
// ❌ 危险：原始指针/引用捕获
struct Task {
    void run() { /* ... */ }
};

std::thread bad_example() {
    Task t;
    // lambda 捕获了 t 的地址，但函数返回后 t 就销毁了
    return std::thread([&]() {   // [&] 捕获了局部变量的引用！
        std::this_thread::sleep_for(std::chrono::seconds(1));
        t.run();  // ❌ t 已销毁 → 野指针 → 未定义行为
    });
}  // t 在这里析构，而线程还在睡梦中...
```

#### 解决方案：`shared_ptr` 按值捕获

```cpp
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

struct _adder {
    int __to_add;
    _adder(int val) : __to_add(val) {}
    void add(int x) { std::cout << x + __to_add << '\n'; }
};

void demo_lambda_thread_safety() {
    std::thread t;
    {
        auto add_p = std::make_shared<_adder>(10);

        // 按值捕获 shared_ptr → 引用计数 +1
        auto lambda2 = [add_p](int x) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            add_p->add(x);
            std::cout << "use_count: " << add_p.use_count() << '\n';
        };

        // 启动线程，lambda（含 shared_ptr 副本）被移到线程中
        t = std::thread(lambda2, 5);

        // 离开作用域，原始 add_p 析构
        // 但 _adder 对象不会销毁 → 线程里的 add_p 副本还在持有一份引用
    }  // 引用计数从 2 → 1（线程中的副本仍在）

    t.join();  // join 后线程结束，lambda 析构，引用计数归零，_adder 自动销毁
}
```

#### 原理

```
线程创建时：
                                引用计数 = 2
add_p (栈上, 即将消亡) ──────┐
                             ├──→  _adder 对象 (堆上)
lambda 内的 add_p 副本 ─────┘       地址: 0x5566...
(在线程的 lambda 对象里)

作用域结束时：
add_p 析构 → 引用计数 = 1    ──→  _adder 对象 仍然存活！
                                       ↑
线程内的 lambda 副本继续持有 add_p

t.join() 后：
线程结束 → lambda 析构 → 引用计数 = 0 → _adder 自动 delete
```

#### 关键要点

| 要点 | 说明 |
|------|------|
| **核心思想** | 不是用互斥锁保护临界区，而是用 `shared_ptr` 的引用计数保证**对象寿命足够长** |
| **为什么安全** | `shared_ptr` 按值捕获时，lambda 持有自己的副本（引用计数 +1），原作用域销毁不影响堆上对象 |
| **对比裸指针** | `[&obj]` 或 `[p]`（原始指针）：作用域结束对象销毁，线程拿到悬空指针 → 未定义行为 |
| **对比 `std::ref`** | `[&obj]` 捕获引用：同样有悬挂风险，除非你能保证线程在对象销毁前 join |
| **适用场景** | 线程池任务提交、异步回调、定时器任务——任何**调用者不等结果**的场景 |
| **注意** | 这只解决生命周期安全，不解决数据竞争（多个线程同时读写仍需互斥锁或原子操作） |

```cpp
// ─── 更通用的模式：类成员函数抛到线程 ───
class Worker {
public:
    void doWork() { /* 耗时操作 */ }
};

void safe_thread_launch() {
    auto worker = std::make_shared<Worker>();

    // 捕获 shared_ptr，线程安全地调用成员函数
    std::thread t([worker]() {
        worker->doWork();
    });

    t.detach();  // 放心 detach，worker 在线程结束前绝不会销毁
    // worker 出了作用域也没关系——线程还在持有它
}
```

---

## 4. `std::function`

### 4.1 概念

`std::function` 是 C++11 引入的**多态函数包装器**，可以存储、复制和调用任何可调用对象——函数指针、仿函数、lambda、`std::bind` 表达式等，只要它们的签名兼容。并且实现了类型擦除，使得不同类型的可调用对象可通过统一的接口进行操作。

```
std::function<void(int)> 的内存布局（典型实现）：
┌──────────────────────────────────────┐
│ 函数指针 / 可调用对象地址             │ 8 字节
├──────────────────────────────────────┤
│ 管理/擦除器函数指针                   │ 8 字节  ← 用于构造/析构/复制
├──────────────────────────────────────┤
│ 小对象缓冲（SBO：Small Buffer Opt.）  │ 16~32 字节
│   ┌────────────────────────────────┐ │
│   │ 小可调用对象直接存在这里       │ │  → 避免堆分配
│   │ (如 lambda 捕获少量变量)       │ │
│   └────────────────────────────────┘ │
└──────────────────────────────────────┘
若可调用对象过大，则 SBO 中存一个堆指针：
┌──────────────────────────────────────┐
│ 堆指针 →  ────────────────────────┐ │
│ 管理/擦除器函数指针               │ │
│ SBO (存堆地址 + 元信息)           │ │
└──────────────────────────────────────┘     │
         │                                    │
         ▼                                    ▼
    ┌──────────────────┐         可调用对象实际存储在堆上
    │ new/malloc 分配  │
    │ lambda/functor   │
    └──────────────────┘
```

### 4.2 基础用法

```cpp
#include <iostream>
#include <functional>
#include <vector>
#include <string>

// ─── 普通函数 ───
void hello(const std::string& name) {
    std::cout << "Hello, " << name << "!\n";
}

// ─── 仿函数 ───
struct Greeter {
    void operator()(const std::string& name) const {
        std::cout << "Greetings, " << name << "!\n";
    }
};

int main() {
    // ─── 统一的 std::function 类型 ───
    std::function<void(const std::string&)> func;

    // 可以指向函数
    func = hello;
    func("Alice");   // Hello, Alice!

    // 可以指向仿函数
    func = Greeter{};
    func("Bob");     // Greetings, Bob!

    // 可以指向 lambda
    func = [](const std::string& name) {
        std::cout << "Hi, " << name << "!\n";
    };
    func("Charlie"); // Hi, Charlie!

    // ─── 可放入容器 ───
    std::vector<std::function<void(const std::string&)>> callbacks;
    callbacks.push_back(hello);
    callbacks.push_back(Greeter{});
    callbacks.push_back([](const std::string& name) {
        std::cout << "Yo, " << name << "!\n";
    });

    for (auto& cb : callbacks) {
        cb("World");
    }

    return 0;
}
```

### 4.3 `std::function` 的运行时多态与开销

```cpp
#include <functional>
#include <iostream>
#include <chrono>

void benchmark_overhead() {
    constexpr int N = 10'000'000;

    // ─── 直接调用 vs std::function 调用 ───
    auto lambda = [](int x) { return x * 2; };

    // 直接调用（编译期确定）
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile int sum1 = 0;
    for (int i = 0; i < N; ++i) sum1 = lambda(i);
    auto t2 = std::chrono::high_resolution_clock::now();

    // 通过 std::function 调用（运行时多态，间接调用）
    std::function<int(int)> func = lambda;
    auto t3 = std::chrono::high_resolution_clock::now();
    volatile int sum2 = 0;
    for (int i = 0; i < N; ++i) sum2 = func(i);
    auto t4 = std::chrono::high_resolution_clock::now();

    auto direct = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto wrapped = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    std::cout << "直接调用: " << direct << "ms\n";
    std::cout << "std::function: " << wrapped << "ms\n";
    std::cout << "开销倍数: " << (double)wrapped / direct << "x\n";
}
```

### 4.4 SBO（Small Buffer Optimization）实测

```cpp
#include <functional>
#include <iostream>

struct Small {
    char data[8];
    void operator()() const {}
};

struct Medium {
    char data[32];
    void operator()() const {}
};

struct Large {
    char data[128];
    void operator()() const {}
};

void demo_sbo() {
    std::cout << "sizeof(std::function<void()>): "
              << sizeof(std::function<void()>) << '\n';

    auto s = [](){};  // 空 lambda，0 捕获
    auto f1 = std::function<void()>(s);

    std::function<void()> f2 = Small{};
    std::function<void()> f3 = Medium{};
    std::function<void()> f4 = Large{};

    // 在 libstdc++ 中 SBO 通常为 16 字节，
    // Small(8) 和 Medium(32) 中 Medium 超出 SBO → 堆分配
    std::cout << "SBO 门限约: ~16 字节\n";
    // 实际可通过 sizeof 验证：
    // f2 内部使用了 SBO（Small = 8 ≤ 16）
    // f3 或 f4 可能触发堆分配（Large = 128 > 16）
}
```

### 4.5 `std::function` 的类型擦除机制

`std::function` 的核心是**类型擦除（Type Erasure）**——它通过虚函数或函数指针机制抹去了具体类型信息。

```cpp
// 简化的 std::function 实现概念：
template <typename Signature>
class function;

template <typename Ret, typename... Args>
class function<Ret(Args...)> {
private:
    struct callable_interface {
        virtual ~callable_interface() = default;
        virtual Ret invoke(Args... args) = 0;
        virtual callable_interface* clone() const = 0;
    };

    template <typename F>
    struct callable_model : callable_interface {
        F f;
        callable_model(F&& f_) : f(std::forward<F>(f_)) {}
        Ret invoke(Args... args) override { return f(args...); }
        callable_interface* clone() const override { return new callable_model(*this); }
    };

    // 实际中会使用 SBO 避免堆分配，这里简化
    callable_interface* ptr_ = nullptr;

public:
    template <typename F>
    function(F&& f) : ptr_(new callable_model<F>(std::forward<F>(f))) {}

    Ret operator()(Args... args) {
        return ptr_->invoke(std::forward<Args>(args)...);
    }
};
```

---

## 5. `std::bind` 与占位符

### 5.1 概念

`std::bind` 是一个函数适配器，将可调用对象与部分参数绑定，生成一个新的可调用对象。占位符 `std::placeholders::_1, _2, ...` 表示延迟提供的参数。

```
std::bind(f, _2, 42, _1) 的含义：
                        ┌───────────────────┐
                        │     bind 对象      │
                        │  ┌─────────────┐  │
 调用时: bind_obj(a, b) │  │ f 指针/副本  │  │  → 存储 f
                        │  ├─────────────┤  │
                        │  │ _2 → b      │  │  → 参数映射表
                        │  │ 42          │  │  → 绑定值
                        │  │ _1 → a      │  │
                        │  └─────────────┘  │
                        └───────────────────┘
                        最终调用: f(b, 42, a)
```

### 5.2 基础用法

```cpp
#include <iostream>
#include <functional>

int add(int a, int b, int c) {
    return a + b + c;
}

int main() {
    using namespace std::placeholders;  // _1, _2, _3...

    // ─── 绑定所有参数 —— 生成无参可调用对象 ───
    auto f1 = std::bind(add, 1, 2, 3);
    std::cout << f1() << '\n';  // 6

    // ─── 绑定部分参数，其余由调用时提供 ───
    auto f2 = std::bind(add, 10, _1, 20);
    std::cout << f2(5) << '\n';  // add(10, 5, 20) = 35

    // ─── 重排参数顺序 ───
    auto f3 = std::bind(add, _2, _1, _3);
    std::cout << f3(10, 20, 30) << '\n';  // add(20, 10, 30) = 60

    // ─── 占位符可重复使用 ───
    auto f4 = std::bind(add, _1, _1, _1);
    std::cout << f4(7) << '\n';  // add(7, 7, 7) = 21

    return 0;
}
```

### 5.3 绑定成员函数

**成员函数有一个隐式的 `this`/对象参数**，因此绑定成员函数时必须提供对象实例：

```cpp
#include <iostream>
#include <functional>

class Printer {
public:
    void print(int id, const std::string& msg) const {
        std::cout << "[" << id << "] " << msg << '\n';
    }
};

int main() {
    using namespace std::placeholders;

    Printer p;

    // ─── 绑定成员函数 + 对象指针 ───
    auto bound1 = std::bind(&Printer::print, &p, _1, _2);
    bound1(1, "hello");   // p.print(1, "hello") → [1] hello

    // ─── 绑定成员函数 + 对象副本 ───
    auto bound2 = std::bind(&Printer::print, p, 100, _1);
    bound2("world");      // p.print(100, "world") → [100] world

    // ─── 绑定成员函数到同一个对象，固定参数 ───
    auto say_hi = std::bind(&Printer::print, &p, 42, "Hi!");
    say_hi();             // p.print(42, "Hi!") → [42] Hi!

    return 0;
}
```

### 5.4 绑定成员变量的 Getter/Setter 效应

```cpp
#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>

struct Point {
    int x, y;
};

void demo_bind_member_variable() {
    std::vector<Point> points = {{1,2}, {3,4}, {5,6}};

    // 绑定到成员变量（通过成员指针）
    auto get_x = std::bind(&Point::x, _1);
    // 等价于: [](const Point& p) { return p.x; }

    auto max_x = std::max_element(points.begin(), points.end(),
        [&](const Point& a, const Point& b) {
            return get_x(a) < get_x(b);
        });

    std::cout << "max x: " << max_x->x << '\n';  // 5
}
```

### 5.5 `std::bind` 的性能与内存

```cpp
#include <functional>
#include <iostream>

void func(int a, double b, const std::string& c) {
    std::cout << a << ", " << b << ", " << c << '\n';
}

void demo_bind_size() {
    // 绑定的对象大小取决于绑定了多少个参数
    auto bound = std::bind(func, 42, _1, "hello");

    std::cout << "sizeof(bound) = " << sizeof(bound) << '\n';
    // 包含：函数指针(8) + 存储的 int(4) + padding(4)
    //      + 占位符元信息(通常小) + string(32) ...
    // 典型值: 48~64 字节

    // 而且 std::bind 会按值存储绑定的参数
    // 如果不想拷贝大对象，可以用 std::ref
    std::string large_str = "a very long string that would be expensive to copy";
    auto bound_ref = std::bind(func, 100, _1, std::ref(large_str));
    // 注意：std::ref(large_str) 存储的是引用包装器（8 字节指针）
    // 而非字符串副本（32+ 字节）
}
```

### 5.6 C++17 之后：优先用 Lambda

```cpp
void prefer_lambda_over_bind() {
    int a = 10;
    double b = 3.14;

    // ❌ 旧风格：std::bind
    auto old_way = std::bind(func, a, _1, std::to_string(b));

    // ✅ 现代风格：lambda（更清晰、更高效、易内联）
    auto new_way = [a, b](auto&&... args) {
        return func(a, std::forward<decltype(args)>(args)..., std::to_string(b));
    };
}
```

---

## 6. 绑定类的成员函数

### 6.1 通过 `std::bind` 绑定成员函数

已在第 5.3 节详细说明。这里补充一个完整示例：

```cpp
#include <iostream>
#include <functional>
#include <vector>

class FileSystem {
public:
    void readFile(const std::string& path, std::function<void(const std::string&)> callback) {
        // 模拟异步读取
        std::string content = "/* " + path + " 的内容 */";
        if (callback) callback(content);
    }

    void onFileLoaded(const std::string& content) {
        std::cout << "文件已加载，长度: " << content.size() << '\n';
    }
};

void demo_member_callback() {
    FileSystem fs;

    // ─── 使用 std::bind 将成员函数绑定为回调 ───
    auto callback = std::bind(&FileSystem::onFileLoaded, &fs, _1);
    fs.readFile("config.json", callback);
    // 输出: 文件已加载，长度: 26
}
```

### 6.2 通过 `std::mem_fn` 绑定成员函数

`std::mem_fn` 比 `std::bind` 更专注：它只创建成员函数的包装器，不涉及参数绑定。

```cpp
#include <iostream>
#include <functional>
#include <vector>
#include <algorithm>
#include <string>

class Task {
public:
    Task(const std::string& name, int priority)
        : name_(name), priority_(priority) {}

    void execute() const {
        std::cout << "执行: " << name_ << " (优先级 " << priority_ << ")\n";
    }
    int priority() const { return priority_; }

private:
    std::string name_;
    int priority_;
};

void demo_mem_fn() {
    std::vector<Task> tasks;
    tasks.emplace_back("任务A", 3);
    tasks.emplace_back("任务B", 1);
    tasks.emplace_back("任务C", 2);

    // ─── mem_fn 包装成员函数 ───
    auto exec_fn = std::mem_fn(&Task::execute);
    for (const auto& t : tasks) {
        exec_fn(t);  // 等价于 t.execute()
    }

    // ─── 配合算法 ───
    auto prio_fn = std::mem_fn(&Task::priority);
    auto highest = std::max_element(tasks.begin(), tasks.end(),
        [&](const Task& a, const Task& b) {
            return prio_fn(a) < prio_fn(b);
        });
    std::cout << "最高优先级: " << highest->priority() << '\n';
}
```

### 6.3 Lambda 绑定成员函数（C++17 推荐方式）

```cpp
void demo_lambda_bind_member() {
    FileSystem fs;

    // Lambda 捕获 this 指针
    auto callback = [&fs](const std::string& content) {
        fs.onFileLoaded(content);
    };

    // 或更简洁：lambda 内部直接调用
    // auto callback = [&](const std::string& content) {
    //     fs.onFileLoaded(content);
    // };
}
```

### 6.4 `std::bind` 绑定不同继承层次的 this

多重继承下，`this` 指针可能需要调整，`std::bind` 自动处理：

```cpp
struct A { void fa() {} };
struct B { void fb() {} };
struct C : A, B {};

void demo_this_adjust() {
    C obj;
    // 绑定到 B::fb，但传入 C*，编译器自动调整指针
    auto b1 = std::bind(&B::fb, &obj);
    b1();  // 内部会 static_cast<B*>(&obj) 调整指针

    // 手工验证偏移
    A* pa = &obj;
    B* pb = &obj;
    std::cout << "C*  = " << &obj << '\n';
    std::cout << "A*  = " << pa   << '\n';  // 通常与 C* 相同
    std::cout << "B*  = " << pb   << '\n';  // 通常 A 部分之后，有偏移
    std::cout << "偏移: " << (char*)pb - (char*)pa << " 字节\n";
}
```

---

## 7. 综合对比与选型建议

### 7.1 各可调用对象特性对比

| 特性 | 函数指针 | 仿函数 | Lambda | `std::function` | `std::bind` |
|------|---------|--------|--------|----------------|------------|
| **C++ 版本** | C++98 | C++98 | C++11 | C++11 | C++11 |
| **是否可保存状态** | ❌ | ✅ | ✅ | ✅（包装的对象） | ✅（绑定值） |
| **占用空间** | 8 字节 | 取决于成员 | 取决于捕获 | 32~64 字节 + 可能堆分配 | 取决于绑定的参数 |
| **调用开销** | 间接调用 | 直接调用（可内联） | 直接调用（可内联） | 间接调用（虚函数/函数指针） | 间接调用 |
| **类型安全** | ✅ | ✅ | ✅ | ✅（签名擦除） | ✅ |
| **运行时多态** | ❌ | ❌ | ❌ | ✅ | ❌ |
| **可转为函数指针** | 本身就是 | ❌ | 仅无捕获时 | ❌ | ❌ |
| **适用场景** | C 回调、极简场景 | 需要状态的算法对象 | 局部短小回调 | 回调注册、事件系统 | 参数预绑定 |

### 7.2 性能金字塔

```
                  最快（零开销）
        ┌──────────────────────┐
        │  直接函数调用         │  编译期确定，完全内联
        ├──────────────────────┤
        │  仿函数 / Lambda     │  编译器知道具体类型，可内联
        ├──────────────────────┤
        │  函数指针            │  间接调用（call [rax]），难内联
        ├──────────────────────┤
        │  std::function       │  类型擦除 + 虚调用 / 函数指针
        ├──────────────────────┤
        │  std::bind           │  多层包装 + 占位符解析
        └──────────────────────┘
                  最慢（高开销）
```

### 7.3 选型决策树

```
需要存储可调用对象？
├── 是
│   ├── 需要运行时替换/类型擦除？
│   │   ├── 是 → std::function
│   │   └── 否 → auto / 模板参数（T&& callable）
│   └── 需要绑定部分参数？
│       ├── 是 → Lambda（C++14+） / std::bind（遗留代码）
│       └── 否 → auto / 模板
│
└── 否（只需调用一次/作为参数传递）
    ├── 需要状态？
    │   ├── 是 → Lambda（有捕获）/ 仿函数
    │   └── 否 → Lambda（无捕获）/ 函数指针 / 仿函数
    │
    └── 作为算法参数（std::sort、std::find_if 等）
        ├── 短小逻辑 → Lambda
        ├── 复杂复用逻辑 → 命名仿函数
        └── C 兼容接口 → 函数指针（或 stateless lambda）
```

### 7.4 典型场景推荐

| 场景 | 推荐方案 | 理由 |
|------|---------|------|
| C 标准库回调（`qsort`、`atexit`） | 函数指针 | API 强制要求 |
| STL 算法谓词（`std::sort`、`std::find_if`） | Lambda（C++11）/ 仿函数（C++98） | 可内联，零开销 |
| 异步回调注册（`std::async`、线程池） | `std::function` + Lambda | 类型擦除，存储方便 |
| 事件系统/观察者模式 | `std::function` 容器 | 统一类型，便于存储 |
| GUI/信号槽绑定 | `std::bind` 或 Lambda | 参数适配/对象绑定 |
| 延迟执行/命令模式 | `std::function` | 存储任意可调用对象 |
| 工厂/策略模式 | 模板参数 / `std::function` | 灵活/类型擦除 |

---

## 8. 附录：内存布局图例汇总

### 8.1 各可调用对象在进程地址空间中的位置

```
高地址
  ┌──────────────────────────────┐
  │        栈（Stack）            │
  │  ┌────────────────────────┐ │
  │  │ lambda 捕获的变量副本   │ │
  │  │ std::function 对象     │ │
  │  │ 仿函数对象             │ │
  │  │ 函数指针变量           │ │
  │  │ std::bind 对象         │ │
  │  └────────────────────────┘ │
  ├──────────────────────────────┤
  │        堆（Heap）            │
  │  ┌────────────────────────┐ │
  │  │ std::function 中大对象  │ │  → 超出 SBO 时
  │  │ Large lambda / 仿函数  │ │     在堆上分配
  │  └────────────────────────┘ │
  ├──────────────────────────────┤
  │     数据段（Data Segment）   │
  │  ┌────────────────────────┐ │
  │  │ 全局函数指针           │ │
  │  │ 静态仿函数             │ │
  │  └────────────────────────┘ │
  ├──────────────────────────────┤
  │     代码段（Text Segment）   │
  │  ┌────────────────────────┐ │
  │  │ 函数体                 │ │  → 所有可调用对象的
  │  │ 成员函数 operator()    │ │     可执行代码均在此
  │  │ lambda 的 operator()   │ │
  │  └────────────────────────┘ │
  └──────────────────────────────┘
低地址
```

### 8.2 完整示例：各种可调用对象的 sizeof

```cpp
#include <iostream>
#include <functional>

int plain_func(int) { return 0; }

struct EmptyFunctor {
    void operator()() const {}
};

struct StatefulFunctor {
    int a;
    double b;
    void operator()() const {}
};

int main() {
    int x = 42;
    double y = 3.14;
    std::string s = "hello";

    std::cout << "=== 函数指针 ===\n";
    std::cout << "sizeof(函数指针): " << sizeof(&plain_func) << '\n';

    std::cout << "\n=== 仿函数 ===\n";
    std::cout << "sizeof(EmptyFunctor): " << sizeof(EmptyFunctor) << '\n';
    std::cout << "sizeof(StatefulFunctor): " << sizeof(StatefulFunctor) << '\n';

    std::cout << "\n=== Lambda ===\n";
    auto l_empty   = []() {};
    auto l_int     = [x]() { return x; };
    auto l_int_ref = [&x]() { return x; };
    auto l_many    = [x, y, s]() { return x + y + s.size(); };

    std::cout << "sizeof(无捕获):      " << sizeof(l_empty)   << '\n';
    std::cout << "sizeof(捕获 int):    " << sizeof(l_int)     << '\n';
    std::cout << "sizeof(引用 int):    " << sizeof(l_int_ref) << '\n';
    std::cout << "sizeof(捕获多个):    " << sizeof(l_many)    << '\n';

    std::cout << "\n=== std::function ===\n";
    std::cout << "sizeof(std::function<void()>): "
              << sizeof(std::function<void()>) << '\n';
    std::cout << "sizeof(std::function<int(int)>): "
              << sizeof(std::function<int(int)>) << '\n';

    std::cout << "\n=== std::bind ===\n";
    auto b1 = std::bind(plain_func, 42);
    auto b2 = std::bind(plain_func, _1);
    auto b3 = std::bind(plain_func, x);
    std::cout << "sizeof(bind(plain, 42)): " << sizeof(b1) << '\n';
    std::cout << "sizeof(bind(plain, _1)): " << sizeof(b2) << '\n';
    std::cout << "sizeof(bind(plain, x)): "  << sizeof(b3) << '\n';

    return 0;
}
```

### 8.3 `std::function` 堆分配 vs SBO 决策流程

```
用户传入可调用对象 f
        │
        ▼
┌─ sizeof(f) ≤ SBO 容量？ ─┐
│        是          否     │
│         │           │     │
│         ▼           ▼     │
│   存入 SBO        new 分配│
│   (栈上)          (堆上)  │
│                         │
│  ┌──────────┐    ┌──────┴──────┐
│  │ SBO buf  │    │ 堆: f 的副本 │
│  └──────────┘    └─────────────┘
└───────────────────────────────┘
```

---

> **总结**：C++ 的可调用对象体系从最轻量的函数指针（8 字节、零开销、无状态）到最灵活的 `std::function`（类型擦除、SBO 优化、运行时多态），覆盖了从底层 C 接口到现代函数式编程的完整需求。推荐原则：
>
> 1. **能用 Lambda 就不用 `std::bind`**——Lambda 更清晰、更易优化
> 2. **`std::function` 有运行时开销**——高频回调路径慎用，可改用模板参数
> 3. **理解内存布局**——捕获/绑定的内容决定了对象大小和堆分配行为
> 4. **无捕获 Lambda 是最优的局部回调**——兼有函数指针的轻量 + 仿函数的可读性

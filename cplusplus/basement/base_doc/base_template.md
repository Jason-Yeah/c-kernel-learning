# C++ 模板（Template）—— 泛型编程

## 概述

模板是 C++ **泛型编程**的核心机制。它允许编写**与类型无关**的代码，由编译器在编译期根据实际使用生成具体实例。模板不是运行时多态（虚函数），而是**编译期多态**——零运行时开销。

---

# 一、函数模板（Function Template）

## 基本用法

函数模板让一个函数适用多种类型，编译器根据实参自动推导类型。

```cpp
template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

int main() {
    std::cout << max(3, 5) << std::endl;       // T 推导为 int
    std::cout << max(3.14, 2.72) << std::endl;  // T 推导为 double
    std::cout << max('a', 'z') << std::endl;    // T 推导为 char
}
```

### 显式指定模板参数

当类型不能自动推导时（如返回类型与参数类型不同），需要显式指定：

```cpp
template<typename T>
T cast_to(double val) {
    // static_cast<T> —— 编译期静态类型转换，无运行时开销。编译器检查类型兼容性，不执行运行时安全检查
    return static_cast<T>(val);
}

int x = cast_to<int>(3.14);          // 显式指定 T = int
double y = cast_to<double>(3.14f);   // 显式指定 T = double
```

## 原理——模板实例化（Template Instantiation）

模板本身**不是**可执行代码，它只是一个"蓝图"。编译器在遇到模板调用时，会根据实参类型**实例化**出具体的函数。

```cpp
// 源码中的模板
template<typename T>
T max(T a, T b) { return a > b ? a : b; }

// 编译器看到 max(3, 5) 后，实例化出：
int max<int>(int a, int b) { return a > b ? a : b; }

// 编译器看到 max(3.14, 2.72) 后，实例化出：
double max<double>(double a, double b) { return a > b ? a : b; }
```

```text
编译流程：

  源码                   编译期                    目标文件
  max(3,5)    ──→   实例化 max<int>      ──→  二进制代码（.text 段）
  max('a','z')──→   实例化 max<char>     ──→  二进制代码（.text 段）

  每种类型生成一份独立的机器码
```

> ⚠️ **代码膨胀**：每种类型实例化出一份独立代码。`max<int>`、`max<double>`、`max<char>` 各有一份函数体。

---

# 二、类模板（Class Template）

## 基本用法

类模板让类的成员类型可参数化，使用时必须**显式指定**类型参数。

```cpp
template<typename T>
class Box {
private:
    T _value;
public:
    Box(const T& val) : _value(val) {}
    T get() const { return _value; }
    void set(const T& val) { _value = val; }
};

int main() {
    Box<int> ibox(42);               // T = int
    Box<std::string> sbox("hello");  // T = std::string
    std::cout << ibox.get() << std::endl;
}
```

## 原理——延迟编译（Lazy Instantiation）

类模板的成员函数只有在**被实际调用时**才会被实例化。

```cpp
template<typename T>
class Demo {
public:
    void foo() { /* 任何类型都能编译通 */ }
    void bar() { T::nonexist(); }  // 只有特定类型才有这个成员
};

Demo<int> d;
d.foo();      // ✅ OK，只实例化 foo()
// d.bar();   // ❌ int::nonexist() 不存在，但没调用就不报错
```

```text
内存布局——类模板实例化后就是一个普通类：

Box<int>    在内存中： ┌──────┐
                      │ int  │ ← 4 字节
                      └──────┘

Box<string> 在内存中： ┌──────────────────────┐
                      │ std::string (32 字节) │ ← ptr + size + cap
                      └──────────────────────┘

模板类本身没有额外开销——就是对应类型的数据成员布局。
```

---

# 三、模板特化（Template Specialization）

让模板对**特定类型**有特殊处理。特化分为**全特化**和**偏特化**，二者的区别在于模板参数被指定了多少。

---

## 全特化（Full Specialization）

所有模板参数都被明确指定，不留任何"待推导"参数。特征：`template<>` 后面没有尖括号参数。

```cpp
// 通用模板
template<typename T>
struct IsVoid {
    static constexpr bool value = false;
};

// 全特化：明确写出 T = void，template<> 后面没有参数了
template<>
struct IsVoid<void> {
    static constexpr bool value = true;
};

std::cout << IsVoid<int>::value;   // false（通用模板）
std::cout << IsVoid<void>::value;  // true（全特化版本）
```

### 函数模板全特化

函数模板只有全特化，**不支持偏特化**。

```cpp
// 通用版本
template<typename T>
const char* type_name() { return "unknown"; }

// 全特化：template<> 后无参数，全部写死在函数名上
template<>
const char* type_name<int>() { return "int"; }

template<>
const char* type_name<double>() { return "double"; }

std::cout << type_name<int>();     // "int"
std::cout << type_name<float>();   // "unknown"（走通用模板）
```

> 函数模板只有全特化，不能偏特化（部分指定参数）。如果要"偏特化"的函数行为，用重载或 tag dispatch 替代。

---

## 偏特化（Partial Specialization）

只指定**部分**模板参数，或对参数加约束（如限定为指针）。仅类模板支持。

```cpp
// 通用模板
template<typename T>
struct IsPointer {
    static constexpr bool value = false;
};

// 偏特化：约束 T 为任意指针类型（T*），T 本身仍待推导
template<typename T>
struct IsPointer<T*> {
    static constexpr bool value = true;
};

std::cout << IsPointer<int>::value;   // false（通用模板）
std::cout << IsPointer<int*>::value;  // true（偏特化版本，匹配 T*）
std::cout << IsPointer<double*>::value; // true
```

```text
全特化 vs 偏特化对比：

                     template<> struct IsVoid<void>
                            ↑ 空的，无待推导参数
                     
                     template<typename T> struct IsPointer<T*>
                            ↑ 还有 T 待推导     ↑ 特征：T* 模式匹配

全特化：template<> struct X<具体类型>
偏特化：template<typename T> struct X<T*>  （对 T 是任意指针做匹配）
偏特化：template<typename T> struct X<T&>   （对 T 是任意引用做匹配）
偏特化：template<typename T, typename U> struct X<T, U>
        template<typename T> struct X<T, int>  （第二个参数固定为 int）
```

## 原理——特化匹配优先级

```text
编译器选择规则：

1. 普通函数（非模板）                ← 最高优先级
2. 全特化模板（template<> 开头）     
3. 偏特化（T* / T& / T, int 等）     ← 仅类模板
4. 主模板（通用模板）               ← 最低优先级
```

---

# 四、模板参数类型

## 类型参数（typename / class）

```cpp
template<typename T, class U>  // typename 和 class 完全等价
class Pair { T first; U second; };
```

## 非类型参数（Non-Type Parameter）

参数可以是**编译期常量**（整型、枚举、指针、引用）：

```cpp
template<typename T, size_t N>
class Array {
    T _data[N];  // N 是编译期常量
public:
    size_t size() const { return N; }
};

Array<int, 100> arr;  // 编译期固定 100 个 int
std::cout << arr.size();  // 100
```

## 模板模板参数（Template Template Parameter）

模板的参数本身也是一个模板：

```cpp
template<typename T,
         template<typename> class Container>
class Wrapper {
    Container<T> _container;  // 比如 vector<int> 或 list<int>
};
```

---

# 五、变参模板（Variadic Templates，C++11）

## 基本用法

接受任意数量、任意类型的参数：

```cpp
// 递归终止条件（无参数时的重载）
void print() {}

// 变参模板
template<typename T, typename... Args>
void print(T first, Args... rest) {
    std::cout << first << " ";
    print(rest...);  // 递归展开
}

print(1, 3.14, "hello", 'x');
// 输出: 1 3.14 hello x
```

## 底层原理——编译期递归展开

```text
print(1, 3.14, "hello")

编译器展开为：
→ print<int, double, const char*>(1, 3.14, "hello")
  → print<double, const char*>(3.14, "hello")
    → print<const char*>("hello")
      → print()   // 终止
```

> 变参模板是**编译期**递归，**零运行时开销**——展开后就是逐次调用普通函数。

---

# 六、模板的编译模型

## 两阶段编译（Two-Phase Name Lookup）

模板的编译分两个阶段：

```text
第一阶段（定义时）：
  ─ 检查不依赖模板参数的语法（括号匹配、分号、关键字等）
  ─ 不检查依赖类型的具体成员是否存在

第二阶段（实例化时）：
  ─ 模板参数确定后，检查所有依赖类型的具体成员
  ─ 生成具体机器码
```

```cpp
template<typename T>
void func(T x) {
    int y = 0;          // 第一阶段检查：✅ 语法正确
    x.nonexist();       // 第一阶段✅（依赖类型，暂不检查）
    nonexsit_func();    // 第一阶段❌（非依赖，立即报错）
}

func(42);  // 第二阶段：检查 int::nonexist() → ❌
```

## 为什么模板定义通常放在头文件？

普通函数：声明放 `.h`，定义放 `.cpp`，链接器负责找符号。

模板：**实例化时需要看到完整定义**。如果定义在 `.cpp` 里，使用模板的 `.cpp` 编译时看不到定义，无法实例化 → 链接错误。

```cpp
// my_template.h
template<typename T>
T max(T a, T b);  // 只有声明

// my_template.cpp
template<typename T>
T max(T a, T b) { return a > b ? a : b; }  // 定义

// main.cpp
#include "my_template.h"
int x = max(3, 5);  // ❌ 链接错误：找不到 max<int> 的实例

// 解决方案：定义也放头文件，或在 .cpp 末尾显式实例化：
// template int max<int>(int, int);  // 显式实例化
```

---

# 七、模板与内存

## 零开销抽象

模板生成的所有代码都在**编译期**完成，运行时没有虚表查找、没有类型检查开销。

```text
Box<int> 的 sizeof：
  - 存储 int → 4 字节
  - 没有 vptr（没有虚函数）
  - 没有类型信息（没有 RTTI）
  - = sizeof(int)

模板类的实例与手写类完全相同，不增加任何内存开销。
```

## 代码膨胀的代价

每种类型实例化产生一份完整代码，可能导致二进制文件膨胀。

```cpp
// 下面的代码会产生 3 份独立的函数体：
std::vector<int>     v1;  // vector<int> 的所有方法
std::vector<double>  v2;  // vector<double> 的所有方法
std::vector<std::string>  v3;  // vector<string> 的所有方法

// 每种 vector 的方法都有独立实现
// 但内部指针逻辑完全相同，只是 sizeof(T) 不同
```

> 有一些优化可以缓解（如 `_M_allocate` 等与类型无关的代码可以复用），但整体上模板使用越多，生成的代码越大。

---

> 参考：`C++ Templates: The Complete Guide`，GCC libstdc++ 源码 `bits/stl_vector.h`、`bits/stl_tree.h`

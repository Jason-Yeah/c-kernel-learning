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

### 全特化 vs 偏特化 速览

| | 全特化 | 偏特化 |
|--|-------|--------|
| **是什么** | 所有模板参数都明确指定，不留待推导参数 | 只指定部分参数，或对参数加约束模式匹配 |
| **特征** | `template<>` 开头，函数名后写死具体类型 | `template<typename T>` 开头，匹配 `T*` / `T&` / 部分固定 |
| **支持范围** | 函数模板 ✅ / 类模板 ✅ | 仅类模板 ✅（函数模板不支持） |
| **用途** | 为某个特定类型提供完全不同的实现（如 `vector<bool>` 位压缩） | 为一类类型提供特化（如所有指针类型走深拷贝、移除引用等） |
| **匹配优先级** | 高于偏特化，低于普通函数 | 低于全特化，高于主模板 |

> 总结：全特化 = 针对"一个具体类型"开特例；偏特化 = 针对"一类类型"做模式匹配。

## 全特化（Full Specialization）

所有模板参数都被明确指定，不留任何"待推导"参数。特征：`template<>` 后面没有尖括号参数。

### 使用场景

**① `std::vector<bool>` —— 标准库最经典的全特化**

```cpp
// 标准库中的 vector<bool> 全特化（简化示意）
namespace std {
    template<typename T, typename Alloc>
    class vector;  // 通用模板

    template<typename Alloc>
    class vector<bool, Alloc> {  // 全特化
        // 不再存储 bool 数组，而是用 bitset 压缩：
        // 每个 bool 只占 1 位（bit），而非 1 字节
        unsigned char* _bits;  // 8 个 bool 挤在一个 byte 里
    public:
        // operator[] 返回的是代理对象（reference），不是 bool&
        // 因为无法对单个 bit 取地址
    };
}

std::vector<bool> flags(100);  // sizeof: ~100 bits ≈ 13 字节
// 对比 std::vector<char> flags(100);  // 100 字节
```

场景就是**空间优化**——当类型为 `bool` 时，标准库用全特化实现了一个压缩版本，将 8 个 bool 打包到 1 字节中。代价是 `operator[]` 返回代理对象而非引用，行为上与其他 `vector<T>` 不完全一致。

```cpp
std::vector<bool> v = {true, false, true};
auto& ref = v[0];  // ❌ 编译错误：返回的是代理对象，不是 bool&
// 正确做法：
bool val = v[0];   // ✅ 隐式转换
```

> 这也是 C++ 标准里最有争议的全特化之一——它带来了空间收益，但打破了 `vector` 的容器契约。

---

**② 自定义类型的 `std::hash` 全特化**

自定义类型想要作为 `unordered_map` / `unordered_set` 的键，就要提供 `hash` 实现。最干净的方式就是全特化 `std::hash`：

```cpp
struct UserID {
    std::string domain;
    uint64_t    seq;
};

// 全特化 std::hash<UserID>
template<>
struct std::hash<UserID> {
    size_t operator()(const UserID& uid) const noexcept {
        // 组合哈希：boost::hash_combine 思路
        return std::hash<std::string>{}(uid.domain)
             ^ (std::hash<uint64_t>{}(uid.seq) << 1);
    }
};

std::unordered_map<UserID, std::string> users;  // ✅ 可以用了
```

场景是**扩展第三方库/标准库**——你无法修改 `std::hash` 的通用模板，但可以通过全特化为自己的类型插入一个专属版本。

---

**③ 序列化中对基本类型的直接写入**

```cpp
// 通用模板：逐个字段反射式序列化（复杂）
template<typename T>
void serialize(std::ostream& os, const T& val) {
    val.serialize(os);  // 要求 T 有 serialize 方法
}

// 全特化：基本类型直接二进制写入（零开销）
template<>
void serialize<int>(std::ostream& os, const int& val) {
    os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

template<>
void serialize<double>(std::ostream& os, const double& val) {
    os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}
```

场景是**性能关键路径分支**——大多数自定义类型需要结构化序列化（JSON/ProtoBuf），但 `int`/`double` 等基本类型直接 memcpy 即可，用全特化走快速路径。

---

**④ 类型萃取（Type Traits）——编译期分类决策**

标准库 `<type_traits>` 大量使用全特化来实现编译期类型判断：

```cpp
// 通用模板：默认不是 signed
template<typename T>
struct is_signed : std::false_type {};

// 全特化：列出所有有符号类型
template<> struct is_signed<int>   : std::true_type {};
template<> struct is_signed<short> : std::true_type {};
template<> struct is_signed<long>  : std::true_type {};
// ... 几十个全特化

template<typename T>
void print_signedness() {
    if constexpr (is_signed<T>::value)
        std::cout << "signed";
    else
        std::cout << "unsigned";
}
```

场景是**编译期反射**——全特化为每种类型"注册"其属性，编译器在编译期就能查到这些信息，生成不同的代码路径，零运行时开销。

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

### 使用场景

**① `std::remove_reference` —— 移除引用是偏特化的经典应用**

```cpp
// 标准库实现（简化）
template<typename T>
struct remove_reference {
    using type = T;  // 通用模板：T 就是 T
};

// 偏特化：匹配左值引用 T&
template<typename T>
struct remove_reference<T&> {
    using type = T;
};

// 偏特化：匹配右值引用 T&&
template<typename T>
struct remove_reference<T&&> {
    using type = T;
};

static_assert(std::is_same_v<
    std::remove_reference<int&>::type, int>);    // int& → int
static_assert(std::is_same_v<
    std::remove_reference<int&&>::type, int>);   // int&& → int
static_assert(std::is_same_v<
    std::remove_reference<int>::type, int>);     // int → int（走通用模板）
```

场景是**类型擦除/规范化**——当你写转发函数（完美转发）时，参数可能是 `int`、`int&`、`int&&`，但业务逻辑只需要底层的 `int`。用偏特化把三种情况统一输出为 `int`。

---

**② 编译期分支：`std::conditional`——根据条件选类型**

```cpp
// 标准库实现（简化）
template<bool Cond, typename T, typename F>
struct conditional {
    using type = T;  // Cond = true 走这个
};

// 偏特化：Cond = false 时
template<typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};

// 使用
using IntType = std::conditional_t<sizeof(int) == 4, int, long long>;
// 当前平台 int = 4 字节 → IntType = int
// 如果换到 int = 2 字节的 16 位平台 → IntType = long long
```

场景是**平台无关代码**——同一份源码在不同编译平台上类型大小可能不同，用 `conditional` 在编译期选择合适的类型，不需要平台宏和 `#ifdef`。

---

**③ `enable_if` —— SFINAE 的基石**

```cpp
// 标准库实现（简化）
template<bool Cond, typename T = void>
struct enable_if {};

// 偏特化：当 Cond = true 时有 type 成员
template<typename T>
struct enable_if<true, T> {
    using type = T;
};

// 使用：只对整数类型启用
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T>
    increment(T val) {
    return val + 1;
}

double x = increment(3.14);  // ❌ double 不是整数，SFINAE 移除该重载
int    y = increment(42);    // ✅ int 是整数
```

场景是**模板重载筛选**——SFINAE（替换失败不是错误）让编译器悄悄移除不匹配的模板，只保留可编译的版本。`enable_if` 通过偏特化在 `Cond = false` 时让 `type` 不存在，触发 SFINAE。

---

**④ 指针 vs 非指针的差异化处理**

```cpp
// 通用版本：调用拷贝构造函数
template<typename T>
class DeepCopy {
public:
    static T copy(const T& val) {
        return T(val);  // 调用拷贝构造
    }
};

// 偏特化：指针类型 → 递归深拷贝
template<typename T>
class DeepCopy<T*> {
public:
    static T* copy(const T* ptr) {
        if (!ptr) return nullptr;
        return new T(*ptr);  // 递归：T 可能又是指针
    }
};

int x = 42;
int* p = &x;
int** pp = &p;

auto c1 = DeepCopy<int>::copy(x);    // 通用：拷贝 int
auto c2 = DeepCopy<int*>::copy(p);   // 偏特化：深拷贝指针
auto c3 = DeepCopy<int**>::copy(pp); // 偏特化：递归深拷贝嵌套指针
```

场景是**智能指针/资源管理库**——对普通类型的拷贝 = 值拷贝，对指针类型的拷贝 = 深拷贝分配新内存，用偏特化在同一接口下区分两种行为。

---

**⑤ `std::tuple` 的递归展开（递归终止通过偏特化实现）**

```cpp
// 前置声明
template<typename... Types>
class tuple;

// 基例：空 tuple
template<>
class tuple<> {};

// 递归定义：至少一个元素
template<typename Head, typename... Tail>
class tuple<Head, Tail...> : private tuple<Tail...> {
    Head _head;
public:
    Head& head() { return _head; }
    // tuple<Tail...> 的成员通过继承访问
};

tuple<int, double, char> t;  // 展开：
// tuple<int, double, char>
//   → 继承 tuple<double, char>
//       → 继承 tuple<char>
//           → 继承 tuple<>（全特化，递归终止）
```

场景是**变参模板递归**——变参模板的展开天然需要递归：每次剥离第一个参数，剩下的继续递归。**递归终止条件通过全特化 `tuple<>` 实现**，而递归"剥离"本身部分由偏特化 `tuple<Head, Tail...>` 处理（这里的 `Tail...` 是变参包匹配，本质上是一种偏特化模式）。

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

# 四、SFINAE（Substitution Failure Is Not An Error）

SFINAE 是 C++ 模板的核心规则之一：**当模板参数替换导致非法类型或表达式时，编译器不报错，只是默默从重载集合中移除该模板，而不是终止编译**。这个"静默移除"机制是实现编译期分支和模板重载筛选的基础。

## 基本规则

```text
模板实例化过程：

1. 编译器看到模板调用，收集所有候选模板
2. 逐个尝试代入模板参数（Substitution）
3. 如果代入产生非法类型/表达式  →  移除该候选（不报错）
4. 如果代入成功                →  保留该候选
5. 从最终剩余候选中选最佳匹配
```

```cpp
// 两个重载模板
template<typename T>
typename T::type foo(T) {          // 要求 T 有 ::type
    std::cout << "has type\n";
    return {};
}

template<typename T>
void foo(T) {                      // 通用版本
    std::cout << "fallback\n";
}

struct WithType {
    using type = int;
};

foo(WithType{});   // "has type"  — WithType::type 存在，第一个模板匹配
foo(42);           // "fallback"  — int::type 不存在，第一个模板被 SFINAE 移除
                   //               只剩第二个，正常编译
```

关键理解：如果编译器对 `int::type` 报错的话，整个编译就失败了。但因为是**模板替换阶段**的失败，SFINAE 规则将其视为"这个模板不适合"并静默移除。

## 触发条件——什么算"替换失败"

合法触发 SFINAE 的失败情形：

| 失败场景 | 示例（`T = int`） |
|---------|----------------|
| 类型不存在 | `typename T::type` — `int::type` 不存在 |
| 模板参数不匹配 | `template<typename U> struct X<T, U>` — 参数数量不对 |
| 无效的表达式 | `decltype(T{} + T{})` — `int{} + int{}` 成立，但如果 T 没有 operator+ 则失败 |
| 无效的数组大小 | `T[-1]` — 负大小 |
| 无效的模板模板参数 | `template<T> void foo()` — `int` 不是模板 |

**不触发 SFINAE**（会在后续阶段报硬错误）的情形：

- 函数体内部的错误（替换已经完成，进入实例化阶段）
- 违反 ODR（单一定义规则）
- `static_assert` 失败（C++20 前）

## `std::enable_if` —— SFINAE 最常用的工具

`std::enable_if` 本身就是一个利用偏特化实现的 SFINAE 触发器（前一节已提到）：

```cpp
// 标准库简化实现
template<bool Cond, typename T = void>
struct enable_if {};                      // Cond = false 时：没有 type

template<typename T>
struct enable_if<true, T> {               // Cond = true 时：有 type
    using type = T;
};
```

### 场景：函数重载筛选

```cpp
// 只对整数类型启用
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T>
    increment(T val) {
    return val + 1;
}

// 只对浮点类型启用
template<typename T>
std::enable_if_t<std::is_floating_point_v<T>, T>
    increment(T val) {
    return val + 0.5;
}

int main() {
    std::cout << increment(42);      // → 43（走整数版本）
    std::cout << increment(3.14);    // → 3.64（走浮点版本）
    // std::cout << increment("hi"); // ❌ 两者都不匹配，编译错误
}
```

### 逐行解析：SFINAE 函数重载是如何工作的

以 `sfinae_demo.h` 中的代码为例，看编译器对 `print_type(10)` 的处理过程：

```cpp
// 候选 1：要求 T 是整数类型
template<typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
print_type(T value);

// 候选 2：要求 T 是浮点类型
template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, void>::type
print_type(T value);
```

**编译器处理 `print_type(10)`（`T = int`）：**

```
步骤 1：推导 T = int
步骤 2：代入候选 1
  → std::is_integral<int>::value → true
  → std::enable_if<true, void>::type → void
  → 签名有效，保留候选                    ✅

步骤 3：代入候选 2
  → std::is_floating_point<int>::value → false
  → std::enable_if<false, void> 没有 ::type 成员
  → 替换失败，SFINAE 静默移除             ✅

步骤 4：剩余唯一候选 → 调用候选 1
```

**关键点**：如果 `::value` 写成了 `::val`（常见拼写错误）：

```
  → std::is_integral<int>::val → 不存在！
  → std::enable_if< 替换失败 , void>::type
  → 编译器报错：'val' is not a member of 'std::is_integral<int>'
```

但这不是 SFINAE 失败——**`is_integral<int>::val` 根本就不是一个合法的表达式**（成员名都不存在），所以两个候选都会报错，不会有SFINAE的静默移除。

### C++17 简化写法：`_v` 和 `_t` 后缀

C++14/17 为标准库类型萃取和 `enable_if` 提供了别名后缀，避免了冗长的 `typename X<T>::type` / `X<T>::value`：

```cpp
// C++11 原始写法             →  C++17 简写
std::is_integral<T>::value   →  std::is_integral_v<T>       // _v 后缀
typename std::enable_if<      →  std::enable_if_t<           // _t 后缀
    std::is_integral<T>::
    value, void>::type            is_integral_v<T>, void>
```

对比 `sfinae_demo.h` 的两种写法：

```cpp
// 写法 A：C++11 原始风格（你现在用的）
template<typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
print_type(T value);

// 写法 B：C++17 精简风格（推荐）
template<typename T>
std::enable_if_t<std::is_integral_v<T>, void>
print_type(T value);
```

> 区别仅在于语法糖，编译结果完全等价。写法 B 少了一半的代码量。

### 常见编译错误排查

| 错误信息 | 原因 | 修复 |
|---------|------|------|
| `'val' is not a member of 'std::is_integral<int>'` | 把 `::value` 写成了 `::val` | 改成 `::value` 或 `_v` |
| `'type' is not a member of 'std::enable_if<false, ...>'` | enable_if 条件为 false 时没有 `::type`，**这正是 SFINAE 触发的正常现象** | 检查是否期望被 SFINAE 移除 |
| `no matching function for call to 'print_type(int&)'` | **所有候选都被 SFINAE 移除了**，没有留下任何匹配函数 | 检查条件写反了（如 `is_floating_point` 对 int）或拼写错误 |
| `'print_type' : ambiguous call` | 多个候选都匹配，编译器不知道选哪个 | SFINAE 条件没有互斥，比如两个模板条件同时为 true |

> **排查口诀**：所有候选都报 `no matching` → 条件是写反了还是拼错了；只有一条报 `'type' is not a member` → 说明 SFINAE 正常工作，是另一条也匹配失败才报的错。

`enable_if` 的三种常见写法：

```cpp
// 写法 1：返回类型（最直观）
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T> func(T val);

// 写法 2：模板参数默认值（常用于构造函数）
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void func(T val);

// 写法 3：函数参数默认值（不干扰返回类型推导）
template<typename T>
void func(T val, std::enable_if_t<std::is_integral_v<T>, int> = 0);
```

## 场景：判断类型是否可调用（Expression SFINAE）

当你要写一个泛型函数，需要判断某个类型是否支持某个操作时，SFINAE 用 `decltype` 检测表达式合法性：

```cpp
// 检测 T 是否支持 ++ 操作
template<typename T, typename = void>
struct has_pre_increment : std::false_type {};

// 偏特化：如果 decltype(++declval<T&>()) 合法，value = true
template<typename T>
struct has_pre_increment<T, std::void_t<decltype(++std::declval<T&>())>>
    : std::true_type {};

std::cout << has_pre_increment<int>::value;        // true（int 可 ++）
std::cout << has_pre_increment<std::string>::value; // false（string 不可 ++）
```

这种模式叫作 **Detection Idiom**（C++17 后标准库提供了 `std::is_detected`，或者更通用的 `std::void_t` 技巧）。

## 场景：标签分发（Tag Dispatch）—— SFINAE 的替代方案

当函数逻辑在 SFINAE 后分支太复杂时，可以用 tag dispatch 替代：

```cpp
// 内部实现：按类型分类
template<typename T>
void do_advance(T& it, int n, std::random_access_iterator_tag) {
    it += n;  // 随机访问迭代器：直接跳
}

template<typename T>
void do_advance(T& it, int n, std::input_iterator_tag) {
    while (n--) ++it;  // 输入迭代器：一次一步
}

// 对外接口：编译器自动选 tag
template<typename T>
void advance(T& it, int n) {
    do_advance(it, n, typename std::iterator_traits<T>::iterator_category{});
}
```

> SFINAE 和 Tag Dispatch 的取舍：**SFINAE 适用于"是否启用某函数"的开关**，Tag Dispatch 适用于**根据类型属性走不同实现路径**。两者目标不同。

## SFINAE vs `if constexpr`（C++17）

C++17 的 `if constexpr` 在**函数体内部**做编译期分支，比 SFINAE 更直观：

```cpp
// SFINAE 方式
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T> process(T val) {
    return val * 2;
}

template<typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> process(T val) {
    return val / 2;
}

// if constexpr 方式（C++17，更简洁）
template<typename T>
auto process(T val) {
    if constexpr (std::is_integral_v<T>) {
        return val * 2;
    } else if constexpr (std::is_floating_point_v<T>) {
        return val / 2;
    }
}
```

```text
对比：

                SFINAE                      if constexpr

时机            替换阶段（函数签名）        实例化阶段（函数体）
作用域          函数签名层                  函数体内部
适用场景        重载筛选、类型开关          同一函数内的分支逻辑
可读性          差（enable_if 嵌套长）      好（普通 if 语法）
错误信息        晦涩                       清晰
现代推荐        库作者写泛型代码时必要      日常开发首选
```

> C++20 的 Concepts 进一步简化了 SFINAE 的应用场景（`requires` 子句），但目前如果你用 C++17，`if constexpr` + `enable_if` 是最现实的组合。

---

# 五、模板参数类型

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

## `decltype` —— 编译期类型推断

`decltype` 在编译期推断表达式的类型，常用于模板中获取不确定的类型。

### 适用场景

**① 推导函数返回类型（配合 `auto` 尾置返回类型）**

```cpp
// 场景：两个不同类型相加，不知道返回类型是什么
template<typename T, typename U>
auto add(const T& a, const U& b) -> decltype(a + b) {
    return a + b;
}
// T = int, U = double 时 → decltype(a + b) = double
```

```text
没有 decltype 的话，你只能手动写：
  template<typename T, typename U>
  ??? add(T a, U b);            // 返回类型写什么？
  // 重载所有 (int,double)、(double,int)、(int,float) ... 不现实
```

**② 模板元编程中的类型萃取**

```cpp
template<typename T>
struct IteratorTraits {
    // 获取迭代器解引用的类型（类似 *it 的类型）
    using reference = decltype(*std::declval<T>());
};

std::is_same_v<IteratorTraits<std::vector<int>::iterator>::reference,
               int&>;  // true：*it 返回 int&
```

**③ SFINAE 中的表达式合法性检测（Expression SFINAE）**

```cpp
// 检测 T 是否支持 ++ 操作
template<typename T, typename = void>
struct has_pre_increment : std::false_type {};

template<typename T>
struct has_pre_increment<T, std::void_t<decltype(++std::declval<T&>())>>
    : std::true_type {};
// decltype(++std::declval<T&>()) → 如果 ++t 合法，就取它的类型
```

### 什么时候该用 `decltype`

| 场景 | 是否适合用 `decltype` | 说明 |
|------|---------------------|------|
| 模板函数返回类型由参数类型决定 | ✅ **非常适合** | `auto func() -> decltype(...)` |
| 已知表达式类型，但名字太长想简写 | ⚠️ 可以用但没必要 | 不如直接用 `using` 别名 |
| 只需要 `auto` 推导（无引用/const） | ❌ 不需要 | `auto` 就够了 |
| 需要完美保留引用/const 语义 | ✅ **必须用** | `decltype(auto)` |
| 运行时类型判断 | ❌ **不能** | 用 `typeid` 或 `dynamic_cast` |

> **核心原则**：当你需要**去掉类型名字但保留它的类型信息**时用 `decltype`。具体来说，就是写模板时不知道表达式具体类型、或类型太复杂不想手写的时候。

---

# 六、变参模板（Variadic Templates，C++11）

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

## 折叠表达式（Fold Expressions, C++17）

C++17 引入了折叠表达式，让变参包可以直接用一元/二元运算符展开，**无需写递归终止函数**。

### 四种折叠形式

| 形式 | 写法 | 展开示例（`args = {1, 2, 3}`） |
|------|------|-------------------------------|
| **一元右折叠** | `(args op ...)` | `(1 op (2 op 3))` |
| **一元左折叠** | `(... op args)` | `((1 op 2) op 3)` |
| **二元右折叠** | `(args op ... op init)` | `(1 op (2 op (3 op init)))` |
| **二元左折叠** | `(init op ... op args)` | `(((init op 1) op 2) op 3)` |

### 使用场景

**① 求和/累加 —— 代替递归展开**

```cpp
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);   // 一元右折叠：(args[0] + (args[1] + args[2]))
}

template<typename... Args>
auto sum_left(Args... args) {
    return (... + args);   // 一元左折叠：((args[0] + args[1]) + args[2])
}

std::cout << sum(1, 2, 3, 4);   // 10
std::cout << sum_left(1, 2, 3, 4);  // 10（加法结合律，结果相同）
```

**左折叠 vs 右折叠的区别**：

```cpp
// 减法能看出差别
template<typename... Args>
auto sub_right(Args... args) { return (args - ...); }  // 右：1 - (2 - (3 - 4)) = -2

template<typename... Args>
auto sub_left(Args... args)  { return (... - args); }   // 左：((1 - 2) - 3) - 4 = -8

std::cout << sub_right(1, 2, 3, 4);  // -2
std::cout << sub_left(1, 2, 3, 4);   // -8
```

**② 打印所有参数 —— 一行代码代替递归**

```cpp
template<typename... Args>
void print_all(Args... args) {
    (std::cout << ... << args) << std::endl;  // 二元左折叠：((cout << a) << b) << c
}

print_all(1, 3.14, "hello");  // 13.14hello（无分隔符）
```

**③ 加分隔符打印**

```cpp
template<typename... Args>
void print_sep(Args... args) {
    ((std::cout << args << " "), ...);  // 逗号表达式右折叠
}

print_sep(1, 3.14, "hello");  // 1 3.14 hello
```

**④ 检查所有条件是否成立（`&&` 折叠）**

```cpp
template<typename... Args>
bool all_true(Args... args) {
    return (... && args);  // 左折叠：((a && b) && c) && ...
}

std::cout << std::boolalpha;
std::cout << all_true(true, true, true);   // true
std::cout << all_true(true, false, true);  // false
```

**⑤ 检查任意条件是否成立（`\|\|` 折叠）**

```cpp
template<typename... Args>
bool any_true(Args... args) {
    return (... || args);  // 左折叠
}

std::cout << any_true(false, false, true);   // true
```

### 原理——展开时机

```text
折叠表达式的展开发生在编译期，等价于手写链式表达式：

(args + ...)    →    (a + (b + (c + d)))       // 一元右折叠
(... + args)    →    (((a + b) + c) + d)       // 一元左折叠

而旧式递归展开则生成 N 个重载函数：

print(1, 3.14, "hello")
  → print<int, double, const char*>(1, 3.14, "hello")
    → print<double, const char*>(3.14, "hello")
      → print<const char*>("hello")
        → print()     ← 递归终止

折叠表达式更简洁，且在运算符满足结合律时（+、*、&&、||），
编译器还可以进一步优化（如 SIMD 向量化）。
```

### 对比：递归展开 vs 折叠表达式

| 维度 | 递归展开（C++11） | 折叠表达式（C++17） |
|------|------------------|-------------------|
| 代码量 | 需要递归函数 + 终止重载 | 一行表达式 |
| 可读性 | 中（需理解递归） | 高（直观的运算符链） |
| 运行时开销 | 零开销 | 零开销 |
| 支持的运算符 | 任意函数体 | 仅限 32 个二元运算符（+、-、\*、/、&&、\|\|、, 等） |
| 复杂逻辑 | 可以写任意函数体 | 只能组合运算符，复杂逻辑需配合逗号表达式 |

> 折叠表达式**不能完全替代**递归展开——当每个参数的处理逻辑不是简单运算符能表达时（如对每个参数调用不同函数、条件分支），仍然需要递归展开或 C++17 `if constexpr`。

---

# 七、模板的编译模型

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

# 八、模板与内存

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

---

# 九、模板元编程（Template Metaprogramming, TMP）

## 什么是模板元编程

**模板元编程**就是在编译期用模板执行计算。模板实例化发生在编译期，而模板参数可以是类型或整型常量，所以可以用模板递归实现循环、用特化实现分支——最终结果编译进二进制，运行时**零开销**。

```text
普通代码（运行时）：       输入 → 程序执行 → 输出
模板元编程（编译期）：     输入（模板参数）→ 编译器展开 → 二进制自带结果
```

```cpp
// 最简单的元函数：编译期计算阶乘
template<int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

// 递归终止：全特化
template<>
struct Factorial<0> {
    static constexpr int value = 1;
};

int main() {
    // 编译期就算好了，运行时就是加载一个常量
    std::cout << Factorial<5>::value;  // 120
}
```

```text
编译器展开 Factorial<5>：

Factorial<5>::value
  → 5 * Factorial<4>::value
    → 5 * 4 * Factorial<3>::value
      → 5 * 4 * 3 * Factorial<2>::value
        → 5 * 4 * 3 * 2 * Factorial<1>::value
          → 5 * 4 * 3 * 2 * 1 * Factorial<0>::value
            → 5 * 4 * 3 * 2 * 1 * 1 = 120 ✅
```

## 核心机制：递归 + 特化

模板元编程只有两个基本工具：

| 工具 | 对应普通编程 | 说明 |
|------|------------|------|
| **递归模板** | 循环 | 用 `N * Factorial<N-1>` 反复实例化 |
| **模板特化** | if 分支 / 终止条件 | `Factorial<0>` 全特化终止递归 |

## 编译期计算 vs 运行时计算

```cpp
// 运行时：每次调用都要算
int factorial_runtime(int n) {
    int r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

// 编译期：二进制里直接存结果
template<int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};
template<> struct Factorial<0> { static constexpr int value = 1; };

int main() {
    int a = factorial_runtime(5);     // 运行时算乘法
    int b = Factorial<5>::value;      // 编译期已算好，等价于 int b = 120;
}
```

```text
反汇编对比：

factorial_runtime(5):
   mov    eax, 1
   mov    ecx, 2
.L2:                                ← 有循环，运行时真正执行
   imul   eax, ecx
   add    ecx, 1
   cmp    ecx, 5
   jle    .L2

Factorial<5>::value:
   mov    eax, 120                   ← 直接是常量，没有循环
```

## 类型计算——模板元编程的真正用途

TMP 不止能算数字，更核心的是在**编译期操作类型**。

```cpp
// 元函数：获取数组的元素类型
template<typename T>
struct RemoveExtent {
    using type = T;           // 非数组类型，type 就是自身
};

template<typename T, size_t N>
struct RemoveExtent<T[N]> {   // 偏特化：匹配 T[N]
    using type = T;           // 去掉维度，留下元素类型
};

// 使用
std::is_same_v<RemoveExtent<int[10]>::type, int>;     // true
std::is_same_v<RemoveExtent<int[5][10]>::type, int[10]>; // true（只去掉最外层）
```

```cpp
// 元函数：添加/移除指针
template<typename T>
struct AddPointer {
    using type = T*;
};

template<typename T>
struct RemovePointer {
    using type = T;           // 通用：T 不是指针
};

template<typename T>
struct RemovePointer<T*> {    // 偏特化：匹配指针，剥离 *
    using type = T;
};

static_assert(std::is_same_v<AddPointer<int>::type, int*>);       // int → int*
static_assert(std::is_same_v<RemovePointer<int*>::type, int>);    // int* → int
static_assert(std::is_same_v<RemovePointer<int>::type, int>);     // int → int（不变）
```

> 这些类型操作的集合构成了 `<type_traits>` 标准库。每个 trait 本质上就是一个**模板元函数**：输入类型，输出类型或编译期常量。

## 编译期分支选择：`std::conditional`

```cpp
// 根据条件在编译期选择类型
template<bool Cond, typename T, typename F>
struct conditional {
    using type = T;
};

template<typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};

// 使用：不同类型的 size_t 在不同平台上宽度不同
using size_type = conditional_t<sizeof(void*) == 8, uint64_t, uint32_t>;
// 64 位平台 → uint64_t；32 位平台 → uint32_t
```

## 模板元编程的进化路线（C++11 → C++17 → C++20）

随着 C++ 标准演进，很多以前必须用 TMP 才能做的事，现在有了更简洁的替代。

| 场景 | 经典的 TMP（C++03/11） | C++17 方式 | C++20 方式 |
|------|----------------------|-----------|-----------|
| 阶乘计算 | `Factorial<5>::value` 递归模板 | `constexpr` 函数 | 同 C++17 |
| 类型判断 | `is_integral<T>::value` | `is_integral_v<T>` | 同 C++17 |
| 条件分支 | `enable_if` + SFINAE | `if constexpr` | `if constexpr` |
| 类型选择 | `conditional<Cond, T, F>::type` | `conditional_t<Cond, T, F>` | `conditional_t` |
| 概念约束 | SFINAE 长难诊断 | SFINAE | `requires` / `concept` |

### `constexpr` 函数——代替编译期数值计算

C++11 引入了 `constexpr`，C++17 大幅放宽了限制：

```cpp
// C++17：比模板递归阶乘简单得多
constexpr int factorial(int n) {
    int r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

static_assert(factorial(5) == 120);   // 编译期断言 ✅

// 当参数是运行时变量时，退化为普通函数
int n;
std::cin >> n;
std::cout << factorial(n);           // 运行时计算
```

```text
对比：

模板元编程阶乘：             constexpr 函数：
template<int N>              constexpr int factorial(int n) {
struct Factorial {               int r = 1;
    static constexpr             for (int i = 2; i <= n; ++i)
        int value = ...               r *= i;
};                             }
template<>
struct Factorial<0> {
    static constexpr int value = 1;
};

Factorial<5>::value            factorial(5)
  编译期、仅整数常量              编译期、支持运行期变量
  代码量大                       简洁
  只能递归                       可以写循环
```

### `if constexpr`——代替 SFINAE 的条件分支

```cpp
template<typename T>
auto print_info(const T& val) {
    if constexpr (std::is_pointer_v<T>) {
        std::cout << "pointer: " << *val;
    } else if constexpr (std::is_integral_v<T>) {
        std::cout << "integer: " << val;
    } else {
        std::cout << "other: " << val;
    }
}
```

不需要两个 SFINAE 重载，不需要 `enable_if`，一个函数内部编译期分支，清晰得多。

## 所以模板元编程现在还有用吗？

```text
仍有不可替代的场景：

1. 类型计算 —— 操作类型本身（AddPointer、RemoveReference 等）
   constexpr 只能操作值，不能操作类型

2. 类型萃取 —— 判断类型的属性（is_integral、is_class）
   必须通过模板偏特化/SFINAE 实现

3. 编译期分发 —— 根据类型信息选择不同实现
   C++20 concept 简化了，底层仍是 TMP

4. 泛型库开发 —— STL、Boost、folly
   库作者必须掌握 TMP，用户只需用 constexpr / if constexpr

已经可以被替代的场景：

1. 编译期数值计算     → constexpr 函数（更简洁）
2. 函数重载筛选      → if constexpr + concept（更清晰）
3. 编译期断言        → static_assert（不需要 TMP）
```

> **一句话总结：如果你写应用代码，日常用 `constexpr` + `if constexpr` + `concept` 就够了。如果你写泛型库，TMP 仍然是基本功。**

# 万能引用、完美转发与值类别

---

# 一、左值与右值

## 定义

| 概念 | 本质 | 能否取地址 | 示例 |
|------|------|-----------|------|
| **左值（lvalue）** | 有名字、有内存地址的表达式 | ✅ 可以 `&x` | `int x = 10;` — `x` 是左值 |
| **右值（rvalue）** | 临时对象、字面量，没有持久地址 | ❌ 不能 `&42` | `10`、`x + 1`、`std::string("hello")` |

```cpp
int x = 10;         // x 是左值
int y = x + 1;      // x 是左值，x + 1 是右值（临时结果）
int* p = &x;        // ✅ &x 合法
// int* p = &42;    // ❌ &42 非法，42 是右值
```

## 为什么需要区分左值和右值

C++11 引入移动语义后，区分左右值变得关键：

- **左值**：有持久状态，不能随便"偷"它的资源（别人可能还在用）
- **右值**：临时对象，即将销毁，可以放心"偷"它的资源

```cpp
std::string a = "hello";
std::string b = a;            // a 是左值 → 拷贝构造（深拷贝）
std::string c = std::move(a); // std::move(a) 是右值 → 移动构造（偷指针）
```

## 纯右值（prvalue）与将亡值（xvalue）

C++11 之后右值进一步细分：

```text
表达式分类（C++11）：

    表达式
    /    \
   左值   右值
   (lvalue)  / \
         纯右值  将亡值
        (prvalue) (xvalue)
```

| 类别 | 说明 | 示例 |
|------|------|------|
| **左值（lvalue）** | 有身份、有地址 | 变量名、`*p`、`++it` |
| **纯右值（prvalue）** | 无身份、纯粹的值 | 字面量 `42`、算术结果 `a+b`、`&x` |
| **将亡值（xvalue）** | 有身份但即将销毁，可移动 | `std::move(x)`、`static_cast<T&&>(x)` |

> 日常开发通常只分"左值"和"右值"就够了。`std::move(x)` 本质上就是 `static_cast<T&&>(x)`——把左值 `x` 转换成一个将亡值（xvalue），标记为"可移动"。

---

# 二、右值引用（Rvalue Reference）

## 基本语法

```cpp
T&& var = 右值;  // 右值引用：只能绑定到右值
```

```cpp
int&& rref = 42;           // ✅ 42 是右值
int x = 10;
// int&& rref2 = x;        // ❌ x 是左值，不能绑定到右值引用
int&& rref3 = std::move(x); // ✅ std::move(x) 是右值
```

## 右值引用的本质

右值引用本身是**左值**（它有名字，可以取地址）：

```cpp
void foo(int&& val) {
    // 这里的 val 是左值——它有名字、有地址
    std::cout << &val;      // ✅ 可以取地址
}
```

这是理解 `std::forward` 的关键：**右值引用类型的参数，在被使用的时候是左值**。

---

# 三、万能引用（Universal Reference / Forwarding Reference）

## 定义

```cpp
template<typename T>
void foo(T&& param);  // T&& 不一定是右值引用——取决于 T 的推导
```

`T&&` 放在模板里，根据传入参数的不同，可以变成 `int&`（左值引用）或 `int&&`（右值引用）：

```cpp
int x = 10;
foo(x);    // T 推导为 int&  →  T&& = int& &&  →  int&  （左值引用）
foo(10);   // T 推导为 int   →  T&& = int&&             （右值引用）
```

## 万能引用 vs 右值引用

```cpp
// 万能引用（模板参数推导上下文）
template<typename T>
void foo(T&& param);      // T&& 可以是左值引用或右值引用

// 右值引用（没有模板推导）
void bar(int&& param);    // int&& 只能是右值引用，不接受左值

// 也不是万能引用（auto&& 也是万能引用）
auto&& x = 42;            // 万能引用
auto&& y = some_lvalue;   // 万能引用——绑定到左值
```

### 判定规则

```cpp
template<typename T>
void func(T&& param);         // ✅ 万能引用：T 是模板参数，param 类型是 T&&

template<typename T>
void func(std::vector<T>&& param);  // ❌ 不是万能引用：有 std::vector<T> 包裹

template<typename T>
void func(const T&& param);   // ❌ 不是万能引用：有 const

// auto&& 也是万能引用
auto&& v = vector;            // ✅ 万能引用
for (auto&& elem : container) // ✅ 范围 for 中的 auto&&
```

## 引用折叠（Reference Collapsing）

万能引用的底层机制是**引用折叠**。C++ 不允许"引用的引用"，但编译器在模板实例化时会自动折叠：

```text
T&   &   →  T&     （左值引用 + 左值引用 = 左值引用）
T&   &&  →  T&     （左值引用 + 右值引用 = 左值引用）
T&&  &   →  T&     （右值引用 + 左值引用 = 左值引用）
T&&  &&  →  T&&    （右值引用 + 右值引用 = 右值引用）

规则：只要有一个是左值引用，结果就是左值引用。
      两个都是右值引用，结果才是右值引用。
```

```cpp
template<typename T>
void foo(T&& param);

int x = 10;
foo(x);   // T = int&  →  T&& = int& &&  →  折叠 → int&  （左值引用）
foo(10);  // T = int   →  T&& = int&&    →  不折叠       （右值引用）
```

---

# 四、类型推导规则

## 模板参数推导（`T&&` 万能引用时）

```cpp
template<typename T>
void foo(T&& param);

int x = 10;
const int cx = 20;

foo(x);      // T = int&         →  param 类型: int&
foo(cx);     // T = const int&   →  param 类型: const int&
foo(10);     // T = int          →  param 类型: int&&
```

### 推导规则对比（`T` vs `T&` vs `T&&`）

```cpp
template<typename T> void by_value(T param);     // 值传递
template<typename T> void by_ref(T& param);      // 左值引用
template<typename T> void by_forward(T&& param); // 万能引用

int x = 10;
const int cx = 20;

by_value(x);       // T = int        param: int          （去除引用和const）
by_value(cx);      // T = int        param: int          （去除引用和const）
by_value(10);      // T = int        param: int

by_ref(x);         // T = int        param: int&
by_ref(cx);        // T = const int  param: const int&
// by_ref(10);     // ❌ 不能绑定左值引用到右值

by_forward(x);     // T = int&       param: int&          （引用折叠）
by_forward(cx);    // T = const int& param: const int&    （引用折叠）
by_forward(10);    // T = int        param: int&&
```

> **关键记忆**：值传递 `T` 去 const、去引用；万能引用 `T&&` 保留所有 cv 限定和引用信息。

---

# 五、`std::move` —— 强制转为右值

## 源码剖析

```cpp
// C++11 标准库实现（简化）
template<typename T>
typename std::remove_reference<T>::type&&
move(T&& t) noexcept {
    // 将 t 强制转换为右值引用
    return static_cast<typename std::remove_reference<T>::type&&>(t);
}
```

逐行拆解：

```cpp
template<typename T>
typename std::remove_reference<T>::type&&
```

- 输入：任意类型 `T`（通过 `T&&` 万能引用接收，左值右值都能接受）
- 输出：`remove_reference<T>::type&&` — 去掉引用后加 `&&`，确保结果是右值引用

```cpp
move(T&& t) noexcept
```

- `T&& t`：万能引用，接收左值时 `T = int&`，接收右值时 `T = int`
- `noexcept`：保证不抛异常，让编译器可以优化

```cpp
return static_cast<typename std::remove_reference<T>::type&&>(t);
```

- `static_cast<...&&>(t)`：强制把 `t` 转成右值引用

### 关键理解

```cpp
int x = 10;
std::move(x);  // T = int&（因为 x 是左值，万能引用推导 T = int&）
               // remove_reference<int&>::type = int
               // 返回类型: int&&
               // static_cast<int&&>(x) → 将 x 转为右值引用

std::move(10); // T = int（右值，T 直接推导为 int）
               // remove_reference<int>::type = int
               // 返回类型: int&&
               // static_cast<int&&>(10) → 10 本来就是右值，什么也没改变
```

**重要**：`std::move` 只是**类型转换**（`static_cast`），**不做任何实质的移动操作**。它只负责把参数标记为"可移动"。真正的移动发生在移动构造函数/移动赋值运算符里。

```text
std::move(x) 干了什么？

    内存视角：
    x（int, 值 10）        std::move(x) 后
    ┌────────┐             ┌────────┐
    │   10   │             │   10   │
    └────────┘             └────────┘
    地址不变                 地址不变

    std::move 只是改变了表达式的"值类别"（value category），
    没有改变内存中的任何比特位。
```

---

# 六、`std::forward` —— 保持值类别原样转发

## 源码剖析

```cpp
// C++11 标准库实现（简化）
template<typename T>
T&& forward(typename std::remove_reference<T>::type& t) noexcept {
    return static_cast<T&&>(t);
}

template<typename T>
T&& forward(typename std::remove_reference<T>::type&& t) noexcept {
    static_assert(!std::is_lvalue_reference<T>::value,
                  "forward of rvalue to lvalue");
    return static_cast<T&&>(t);
}
```

对于实际使用，只看第一个重载就够（第二个是右值版本的保护重载）：

```cpp
template<typename T>
T&& forward(typename std::remove_reference<T>::type& t) noexcept {
    return static_cast<T&&>(t);
}
```

### 行为拆解

```cpp
template<typename T>
void wrapper(T&& arg) {
    // arg 在这里是左值（它有名字）
    // 如果直接传 arg 给下游，永远是左值——丢失了原始的值类别
    
    // 完美转发
    foo(std::forward<T>(arg));
    // 当 T = int&  →  forward<int&>(arg)  →  int& &&   →  int&  （左值引用）
    // 当 T = int   →  forward<int>(arg)   →  int&&              （右值引用）
}
```

### 逐层拆解 `std::forward`

```cpp
// 情况 1：传入左值
int x = 10;
wrapper(x);   // T = int&

// forward<int&>(arg):
//   1. T = int&
//   2. T&& → int& && → 折叠为 int&
//   3. static_cast<int&>(arg) → 返回左值引用 ✅

// 情况 2：传入右值
wrapper(10);  // T = int

// forward<int>(arg):
//   1. T = int
//   2. T&& → int&&
//   3. static_cast<int&&>(arg) → 返回右值引用 ✅
```

## `move` vs `forward` 对比

```cpp
// move：无条件的右值转换
template<typename T>
remove_reference_t<T>&& move(T&& t) {
    return static_cast<remove_reference_t<T>&&>(t);
}

// forward：有条件的右值转换（保留原始值类别）
template<typename T>
T&& forward(remove_reference_t<T>& t) {
    return static_cast<T&&>(t);
}
```

```text
                 std::move                    std::forward
                 
作用          无条件转为右值              根据原始值类别决定
使用场景      "我确定这个对象可以移动"     "我不确定，保持原样传给下一个函数"
参数          万能引用（T&&）             左值引用（T&），需要显式指定 T
调用方式      std::move(x)                std::forward<T>(arg)
安全性        "主动放弃"                  安全保留原始值类别
```

> **口诀**：`move` 没有条件，`forward` 根据条件。`move` 告诉编译器"我放弃这个值"，`forward` 告诉编译器"我不知该不该放弃，按来源处理"。

---

# 七、完美转发（Perfect Forwarding）

## 为什么需要完美转发

没有完美转发时，模板函数会丢失参数的左值/右值属性：

```cpp
template<typename T>
void wrapper(T arg) {     // 值传递——丢失左右值信息
    foo(arg);             // arg 永远是左值
}
```

```cpp
struct Heavy {
    std::string data;
    Heavy() = default;
    Heavy(const Heavy&) { std::cout << "copy\n"; }
    Heavy(Heavy&&) { std::cout << "move\n"; }
};

Heavy h;
wrapper(h);         // 两次拷贝：传入是拷贝，传给 foo 又是拷贝
wrapper(Heavy{});   // 一次构造+一次拷贝：应该可以移动，结果变成拷贝
```

## 完美转发的正确写法

```cpp
template<typename T>
void wrapper(T&& arg) {                    // 万能引用
    foo(std::forward<T>(arg));              // 完美转发
}

Heavy h;
wrapper(h);         // foo(h)                → 拷贝（原始是左值）
wrapper(Heavy{});   // foo(Heavy{})          → 移动（原始是右值）
```

## 可变参数完美转发

```cpp
template<typename... Args>
auto make_shared(Args&&... args) {          // 万能引用包
    return std::make_shared<T>(
        std::forward<Args>(args)...          // 逐个完美转发
    );
}

// 展开过程：
// std::forward<Args>(args)...
// → std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), ...
```

## 完美转发失败的情况

```cpp
// 情况 1：花括号初始化
template<typename T>
void wrapper(T&& arg) {
    foo(std::forward<T>(arg));
}

wrapper({1, 2, 3});  // ❌ 花括号初始化不能推导模板参数
wrapper(std::vector<int>{1, 2, 3});  // ✅ 显式构造

// 情况 2：0 或 NULL 作为空指针
wrapper(0);           // T = int，不是空指针
wrapper(nullptr);     // ✅ 正确使用 nullptr

// 情况 3：只声明了类型但没有定义的静态 const 成员
struct S { static const int value = 42; };
wrapper(S::value);    // ⚠️ 如果 S::value 只有声明没有定义，链接错误
```

---

# 八、底层视角：汇编层面理解

看一个最小例子理解 `move` 和 `forward` 的底层：

```cpp
struct Data {
    char* buf;
    size_t len;
};

// 拷贝
void copy(Data& d) {
    // 深拷贝 buf
}

// 移动
void move_impl(Data&& d) {
    d.len = 0;    // 偷走资源后，清空源对象
}
```

## 运行时对比

```text
              拷贝                                   移动

    ┌──────────┐                               ┌──────────┐
    │ 源对象    │                               │ 源对象    │
    │ buf ─────┼────┐                          │ buf ─────┼────┐
    │ len=100  │    │                          │ len=0    │    │
    └──────────┘    │                          └──────────┘    │
                    │     堆内存                              │
    ┌──────────┐    │     ┌────────┐          ┌──────────┐    │
    │ 目标对象  │    │     │ data   │          │ 目标对象  │    │
    │ buf ─────┼────┘     │ 100B   │          │ buf ─────┼────┘
    │ len=100  │          └────────┘          │ len=100  │
    └──────────┘                               └──────────┘
    
    深拷贝：分配新内存 + 复制 100B          移动：复制指针 + 置空源对象
    O(n) 时间复杂度                          O(1) 时间复杂度
```

## `std::move` 的汇编层面

编译器对 `std::move` 的优化：

```cpp
// 源码
void bar(Data&& d);
Data d;
bar(std::move(d));

// 实际汇编（经过优化后）
// std::move 的 static_cast 在大多数实现中不产生任何指令
// 它只影响编译器的类型系统和后续的重载决议
// 所以通常：bar(d) 和 bar(std::move(d)) 在汇编层没有区别
// 区别在于 bar 内部选择了哪个重载（拷贝构造 vs 移动构造）
```

```text
std::move 本身不产生任何机器码——它是一个"编译期概念转换"。
实际生成的代码差异在于被调函数内部选择的构造函数：

  foo(T&)       → 内部调用拷贝构造（有 malloc + memcpy）
  foo(T&&)      → 内部调用移动构造（只有指针赋值 + 置空）
  
  差异发生在被调函数内部，而不是 move 调用处。
```

## `std::forward` 的汇编层面

```cpp
template<typename T>
void wrapper(T&& arg) {
    // 没有 forward：arg 是左值
    foo(arg);         // 永远调用 foo(T&)
    
    // 有 forward：arg 恢复原始值类别
    foo(std::forward<T>(arg));  // 左值→foo(T&)，右值→foo(T&&)
}
```

```text
forward<T>(arg) 的汇编影响：

case 1: T = int&（传入左值）
  forward<int&>(arg)  →  static_cast<int&>(arg)
  → 没有类型转换，arg 本身已经是 int&
  → 零开销

case 2: T = int（传入右值）
  forward<int>(arg)   →  static_cast<int&&>(arg)
  → 汇编层面也是零指令——仅影响后续重载决议
  → 零开销
```

> **结论**：`move` 和 `forward` 都是**零开销抽象**——它们在汇编层面不产生额外指令，只影响编译器的类型系统和重载选择。

---

# 九、总结

## 快速记忆

```text
左值引用（T&）         → 绑定到左值，不能绑定到右值
右值引用（T&&）        → 绑定到右值，不能绑定到左值
万能引用（模板T&&）     → 左值右值都能绑定，类型由推导决定
引用折叠              → 两个引用变一个，有左值引用就折成左值引用

auto&&               → 也是万能引用（范围 for、lambda 泛型参数）

move(x)              → 无条件的 static_cast<T&&>(x)，告诉编译器"可移动"
forward<T>(arg)      → 有条件的 static_cast<T&&>(arg)，保持原始值类别

移动语义的核心        → 不复制资源，偷指针，置空源对象
完美转发的目的         → 模板函数中保留参数的原始值类别
```

## 使用场景一览

| 场景 | 写法 | 说明 |
|------|------|------|
| 移动构造/赋值 | `T(T&& other) noexcept` | 接收右值，偷资源 |
| 无条件移动 | `std::move(x)` | 主动放弃 x |
| 完美转发 | `std::forward<T>(arg)` | 保持 arg 原始值类别 |
| 转发引用参数 | `template<typename T> void foo(T&& arg)` | 万能引用接收 |
| 容器中 emplace | `emplace_back(std::forward<Args>(args)...)` | 原位构造 |
| 工厂函数 | `make_shared<T>(std::forward<Args>(args)...)` | 完美转发构造参数 |

---

> 参考：C++11 标准草案 N3291、`<type_traits>` / `<utility>` 源码、Effective Modern C++ Item 23-30

# Effective C++：条款 41～48 学习笔记

> 对应《Effective C++》（第三版）第七章 **Templates and Generic Programming（模板与泛型编程）**。
>
> 模板最难的地方不是尖括号，而是编译时机：编译器先读取模板定义，等具体类型出现后再实例化。某些名称是否为类型、基类到底拥有哪些成员、隐式转换能否参加推导，都受到这个“两阶段”过程影响。
>
> 本文以原书原则为主，补充 C++11～C++20 的 type traits、`if constexpr`、concepts、可变参数模板和标准智能指针。示例默认使用 C++17；C++20 concepts 示例会单独标明。

---

## 先建立模板编译模型

```cpp
template <class T>
void printTwice(const T& value) {
    std::cout << value << value;
}

printTwice(42);              // 实例化 printTwice<int>
printTwice(std::string{"A"}); // 实例化 printTwice<std::string>
```

可用下面的简化过程理解：

```text
读取模板定义
  → 检查不依赖 T 的语法和名称
  → 暂时保留依赖 T 的表达式

调用 printTwice(42)
  → 推导 T = int
  → 形成 printTwice<int>
  → 检查 int 是否支持需要的操作
  → 生成或复用相应机器码
```

“形成一份具体版本”常称为模板实例化。编译器不一定真的保留每份重复机器码；优化器和链接器可能合并，但源码设计不应把希望全部寄托在后端去重上。

## 缩写与术语速查

| 术语 | 含义 |
| --- | --- |
| TMP | Template Metaprogramming，模板元编程 |
| traits | 特性类：在编译期提供某个类型的属性或关联类型 |
| dependent name | 依赖名称：其含义依赖模板参数的名称 |
| instantiation | 实例化：用具体模板实参形成具体实体 |
| specialization | 特化：为部分或全部模板实参提供专门定义 |
| SFINAE | Substitution Failure Is Not An Error，替换失败不立即构成整个程序错误；常用于旧式条件重载 |
| concept | C++20 的具名约束，描述模板参数必须满足的操作/关系 |
| tag dispatch | 标签分派：用空标签类型和重载在编译期选择实现 |
| ADL | Argument-Dependent Lookup，实参依赖查找 |
| CTAD | Class Template Argument Deduction，C++17 类模板实参推导 |

---

## 条款 41：了解隐式接口和编译期多态

### 普通面向对象接口是显式的

```cpp
class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw() const = 0;
};
```

继承 Drawable 的类明确承诺提供 `draw()`。通过 `Drawable&` 调用时，运行期根据对象动态类型进行虚派发，这叫运行期多态。

### 模板接口由表达式隐式形成

```cpp
template <class T>
void renderTwice(const T& object) {
    object.draw();
    object.draw();
}
```

这里没有要求 T 继承某个基类，但函数体隐含要求：

- `object.draw()` 是合法表达式；
- `draw()` 能在 const T 上调用；
- 返回结果无需被使用。

Circle 和 Button 即使毫无继承关系，只要都满足这些表达式，就能使用模板。编译器分别形成 `renderTwice<Circle>` 和 `renderTwice<Button>`，调用目标通常在编译期已知，因而容易内联。这叫编译期多态。

### “隐式接口”不只看函数签名

```cpp
template <class T>
bool isLarge(const T& value) {
    return value.size() > 10;
}
```

实际约束包括 `size()` 可调用，以及它的结果能与整数 10 使用 `>`。返回类型不必恰好是 `std::size_t`；只需整个表达式能产生可转换为 bool 的结果。这种按表达式判断的思想也叫 duck typing。

### C++20 concepts：把隐式要求写进接口

```cpp
template <class T>
concept DrawableLike = requires(const T& value) {
    value.draw();
};

template <DrawableLike T>
void renderTwice(const T& object) {
    object.draw();
    object.draw();
}
```

concepts 能让错误更靠近调用点、参与重载选择，并把接口意图写清。`requires` 能验证语法表达式，却不会自动证明语义，例如 `draw()` 是否真的画了对象。语义契约仍需要设计、文档和测试。

### 两类多态的选择

| 维度 | virtual 运行期多态 | template 编译期多态 |
| --- | --- | --- |
| 类型集合 | 可在运行时混合派生对象 | 调用时类型通常编译期已知 |
| 分派 | 常见实现经 vptr/vtable 间接调用 | 每个实例化直接调用，常可内联 |
| 容器 | `vector<unique_ptr<Base>>` 可装不同派生类型 | 不同 T 是不同静态类型，需 variant/类型擦除等统一 |
| 成本 | 间接调用与对象布局开销 | 编译时间、代码膨胀风险 |

不要仅凭“模板更快”选择。插件、运行时配置和异构对象适合虚接口；算法对任意满足约束的类型工作时，模板更自然。

---

## 条款 42：了解 typename 的双重意义

### 第一种：声明模板类型参数

```cpp
template <typename T>
class Box {};

template <class T>
class OtherBox {};
```

在这种位置，`typename` 和 `class` 等价；T 可以是 `int`，并不要求它真是 class。团队选一致风格即可。

### 第二种：告诉编译器依赖名称是类型

```cpp
template <class Container>
void printFirst(const Container& container) {
    typename Container::const_iterator it = container.begin();
    std::cout << *it << '\n';
}
```

`Container::const_iterator` 依赖模板参数 Container。读取模板定义时，编译器不知道某个特化是否会把 `const_iterator` 定义成静态数据成员。C++ 在这种有歧义的依赖限定名称前默认不把它当类型；`typename` 明确消除歧义。

```cpp
template <class T>
struct Strange {
    static int value;
};

// T::value * pointer;
```

上面一行既可能意图声明指针，也可能是乘法表达式。模板定义阶段不能凭命名习惯猜测，语言要求程序员标明类型。

### 常见位置

```cpp
template <class Container>
using Value = typename Container::value_type;

template <class Container>
typename Container::const_reference first(const Container& c) {
    return c.front();
}
```

`typename` 不能随意放在基类列表或成员初始化列表的基类名称前。现代标准在部分明确只能出现类型的上下文中放宽了书写要求，但初学时遇到依赖嵌套类型，先确认上下文；编译器提示 “need typename before dependent type” 时不要盲加，先判断名称是否真的应为类型。

### `template` 消歧义关键字

类似问题还存在于依赖对象的成员模板：

```cpp
template <class T>
void call(T& object) {
    object.template convert<int>();
}
```

编译器需要 `template` 才能把 `<` 解析为模板实参列表开头，而不是小于号。这不是条款标题，但与 `typename` 来自同一根因：第一次读取模板时信息不足。

---

## 条款 43：学习处理模板化基类内的名称

### 为什么派生模板看不到基类成员

```cpp
template <class Company>
class MessageSender {
protected:
    void sendClear(const std::string& text);
};

template <class Company>
class LoggingSender : public MessageSender<Company> {
public:
    void send(const std::string& text) {
        // sendClear(text); // 可能找不到
    }
};
```

`MessageSender<Company>` 是依赖基类。编译器第一次读取 LoggingSender 时不能假定所有 Company 对应的基类都有 sendClear，因为模板可能有特化：

```cpp
template <>
class MessageSender<CompanyWithoutClearText> {
    // 特化版本可能根本没有 sendClear。
};
```

因此不会把依赖基类成员自动纳入当前非依赖名称的查找。这种规则让错误不会被错误地提前接受。

### 三种告诉编译器的方法

```cpp
this->sendClear(text);                       // 1. 表明成员依赖当前实例
using MessageSender<Company>::sendClear;    // 2. 将名称引入派生作用域
MessageSender<Company>::sendClear(text);    // 3. 显式限定基类
```

三者并不完全相同：

- `this->` 最常用，保留虚派发；若实例化后的基类没有该成员，会在实例化时报告。
- `using` 适合希望在派生作用域暴露一组基类重载，也连接条款 33 的名称隐藏问题。
- 显式限定调用会抑制虚调用机制；需要刻意调用基类实现时才采用。

现代 C++ concepts 可以提前约束 Company，但并不会取消 dependent base 的名称查找规则。

---

## 条款 44：将与参数无关的代码抽离 templates

### 模板代码膨胀怎样发生

```cpp
template <class T, std::size_t N>
class SquareMatrix {
public:
    void makeIdentity();
};
```

`SquareMatrix<double, 2>` 与 `SquareMatrix<double, 100>` 是不同类型。若 `makeIdentity` 的完整循环都写在类模板中，编译器可能为每个 N 生成近似机器码，只是循环上限不同。

可以把只依赖 T 和运行期 size 的公共算法抽出：

```text
SquareMatrix<T, N>::makeIdentity()
      → MatrixCore<T>::makeIdentity(data, N)

N=2、N=3、N=100
      → 共享同一类仅依赖 T 的算法实现
```

### 还应抽离什么

- 不依赖任何模板参数的代码：普通非模板函数。
- 只依赖部分模板参数的代码：参数更少的辅助模板或基类。
- 相同表示的指针模板：尽可能让公共工作基于 `void*` 或非拥有字节视图，但必须维护类型、对齐和生命周期安全。
- 布尔或整数非类型模板参数只影响少量分支时：考虑运行期参数，或用 `if constexpr` 仅保留必要路径。

### 抽离也有代价

编译期常量 N 可能帮助展开循环、向量化或删除边界检查；改成运行期 size 可能失去优化。现代链接器也可能合并相同代码。正确做法是先消除明显重复，再测量热点、二进制大小与编译时间，而不是让所有模板都转成运行期多态。

---

## 条款 45：运用成员函数模板接受所有兼容类型

### 一个实例化并不是另一个实例化的派生类

`SmartPtr<Derived>` 和 `SmartPtr<Base>` 是两个独立类型。即使 Derived* 能转换为 Base*，包装类之间不会自动获得同样的转换关系。

```cpp
template <class T>
class PtrView {
public:
    explicit PtrView(T* pointer) : pointer_(pointer) {}

    template <class U,
              class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    PtrView(const PtrView<U>& other) : pointer_(other.get()) {}

    T* get() const { return pointer_; }
private:
    T* pointer_;
};
```

构造函数自身是模板，所以 `PtrView<Base>` 可以从 `PtrView<Derived>` 构造。约束 `U* → T*` 必须成立，因而反向转换被拒绝；它也保留 public 继承、const 等原始指针转换规则。

### 为什么不能接受任意 U

若没有约束，错误可能直到构造函数体中给 `pointer_` 赋值时才产生，诊断冗长，也可能意外接受危险的用户自定义转换。C++17 常用 `enable_if`；C++20 可写：

```cpp
template <class U>
    requires std::convertible_to<U*, T*>
PtrView(const PtrView<U>& other) : pointer_(other.get()) {}
```

### 成员模板不会取代正常复制函数

模板构造函数不是拷贝构造函数。若类需要控制同类型复制、移动、赋值，仍应分别 `= default`、实现或 `= delete`。标准智能指针就是这一思想的成熟实现；实际所有权代码优先使用它们，示例 PtrView 只是非拥有教学包装。

---

## 条款 46：需要类型转换时，在 class template 内定义 non-member 函数

### 普通函数模板为什么无法先转换再推导

```cpp
template <class T>
class Rational;

template <class T>
Rational<T> operator*(const Rational<T>& left,
                      const Rational<T>& right);

Rational<int> half{1, 2};
// half * 2; // 模板实参推导阶段不会把 2 转为 Rational<int> 后再推导。
```

函数模板调用先从实参推导 T。第一参数推得 T=int，第二实参却是 int，不匹配 `Rational<T>`；用户定义转换不参加这一推导步骤。条款 24 的非模板 Rational 没有“先推导 T”的门槛，所以两者情况不同。

### hidden friend 如何解决

```cpp
template <class T>
class Rational {
public:
    Rational(T numerator = 0, T denominator = 1);

    friend Rational operator*(const Rational& left,
                              const Rational& right) {
        return Rational(left.numerator_ * right.numerator_,
                        left.denominator_ * right.denominator_);
    }
private:
    T numerator_;
    T denominator_;
};
```

实例化 `Rational<int>` 时，类内 friend 定义产生一个使用具体 Rational<int> 参数的普通函数，而不是等待推导 T 的函数模板。ADL 根据 half 的类型找到它；随后普通重载决议允许把整数 2 转为 Rational<int>。

它被称为 hidden friend：函数属于外围 namespace，却通常只经 ADL 找到。定义在类内还获得 private 访问权，并隐含 inline，需遵守 ODR 的相同定义要求。

若实现很大，可让类内 friend 只做短转发，把实际计算交给 private 成员或辅助函数，控制头文件代码量。

---

## 条款 47：使用 traits classes 表现类型信息

### traits 解决什么问题

泛型算法需要根据类型能力选实现，但不应靠运行期 `if` 猜测。迭代器就是典型例子：vector 迭代器能 `it += n`，list 迭代器只能逐步 `++it`。

标准库通过 `std::iterator_traits<Iterator>::iterator_category` 提供关联类型。它还可为原始指针特化，因此算法不要求迭代器必须拥有嵌套 typedef。

### 标签分派过程

```text
advanceBy(iterator, n)
  → iterator_traits 取得 iterator_category
  → 构造一个空标签对象
  → 重载决议选择对应实现
       random_access_iterator_tag → it += n
       input_iterator_tag         → 循环 ++it
```

标签类型通常没有运行期数据；它们让类型系统在编译期选重载。标准迭代器标签存在继承关系，所以更强类别可匹配较基础实现。

### 现代写法

C++17 的 `if constexpr` 可在模板内按 trait 分支，未采用的分支不会被实例化：

```cpp
if constexpr (std::is_integral_v<T>) {
    // 整数路径
} else {
    // 其他类型路径
}
```

C++20 迭代器 concepts 可以直接约束重载，例如 `std::random_access_iterator<It>`。traits 仍广泛用于关联类型和布尔属性。优先使用标准 `std::type_traits`、iterator traits 和 concepts，不要为已有能力重新设计一套标签。

traits 描述的是编译期属性，不应把会随对象值改变的信息放进去；“这个类型是否整数”适合，“这个 vector 当前是否为空”不适合。

---

## 条款 48：认识 template metaprogramming

### 什么是 TMP

模板元编程让编译器在生成程序前执行计算或选择类型。传统 TMP 使用模板实例化递归：

```cpp
template <unsigned N>
struct Factorial {
    static constexpr unsigned value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {
    static constexpr unsigned value = 1;
};

static_assert(Factorial<5>::value == 120);
```

`Factorial<5>` 需要 `Factorial<4>`，逐层实例化直到全特化 `Factorial<0>`，它相当于递归终止条件。结果可用于数组长度、模板参数或 `static_assert`。

### 它为什么能工作

模板选择、实例化和常量表达式求值发生在编译期。编译失败就不会产生可执行程序；成功后结果可能直接成为常量，运行期没有递归调用栈。

```text
源代码
  → 模板实例化 / constexpr 求值
  → 生成具体类型与函数
  → 优化和机器码
  → 程序运行
```

模板系统足以表达分支、递归、状态和计算，因此可执行复杂编译期程序。但这会消耗编译时间和内存，错误信息也可能出现很深的实例化链。

### 现代 C++ 常有更清楚的工具

```cpp
constexpr unsigned factorial(unsigned n) {
    unsigned result = 1;
    for (unsigned i = 2; i <= n; ++i) result *= i;
    return result;
}

static_assert(factorial(5) == 120);
```

- `constexpr`：函数可以在编译期求值，也可在运行期接受普通值。
- `consteval`（C++20）：每次调用都必须产生编译期结果。
- `if constexpr`：按类型条件丢弃不采用的分支。
- concepts：声明约束，并参与模板选择。
- type traits：查询或变换类型。

传统递归 TMP 仍存在于库实现和类型列表算法，但日常代码应优先选择更易读的 constexpr 函数和标准 traits。

### 适合与不适合的任务

适合：单位和维度检查、序列化表生成、固定查表、类型适配、针对 CPU 能力选择实现、在编译期拒绝不合法组合。

不适合：依赖用户输入、网络或文件内容的普通运行期业务；以及为了“炫技”把清晰循环改成深层模板递归。还要防止每组模板实参生成大量机器码，连接条款 44 的代码膨胀问题。

---

## 条款之间的逻辑链

```text
模板通过表达式形成隐式接口（41）
        ↓
第一次读取模板时，有些名称含义未知
        ├─ 嵌套名称可能是类型：typename（42）
        └─ 依赖基类成员暂不查找：this-> / using（43）
        ↓
不同实参产生不同实例化
        ├─ 抽离无关代码，控制膨胀（44）
        └─ 成员模板建立兼容实例化转换（45）
        ↓
模板推导不会先做用户定义转换
        └─ hidden friend 恢复合理二元转换（46）
        ↓
编译期按类型选择行为
        ├─ traits / tag dispatch / concepts（47）
        └─ TMP / constexpr 计算（48）
```

## 学习检查清单

1. 能否列出某个函数模板对 T 的所有表达式要求？
2. `Container::value_type` 是否依赖模板参数，当前上下文是否需要 `typename`？
3. 派生模板调用依赖基类成员时，为什么需要 `this->` 或 `using`？
4. 模板中是否混入了与某些模板参数无关的大段代码？
5. 不同包装类型之间的转换方向是否由底层指针转换约束？
6. 函数模板推导失败是否误以为会先执行用户定义转换？
7. 选择实现依据的是类型属性还是运行期对象状态？
8. 编译期计算是否真正改善安全/性能，编译成本是否可接受？

## 配套可执行示例

代码位于 [`code/item_41to48`](../code/item_41to48/)。每个文件独立包含 `main`，不要把八个 `.cpp` 一起链接。

普通示例使用：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 item43_dependent_base.cpp -o /tmp/item43
/tmp/item43
```

条款 41 的 concepts 扩展使用 C++20：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -g -O0 item41_implicit_interface.cpp -o /tmp/item41
/tmp/item41
```

| 条款 | 文件 | 观察重点 |
| --- | --- | --- |
| 41 | `item41_implicit_interface.cpp` | 无共同基类的类型满足同一模板表达式；C++20 concept 检查 |
| 42 | `item42_typename.cpp` | dependent nested type 前的 typename |
| 43 | `item43_dependent_base.cpp` | `this->` 与 `using` 访问依赖基类成员 |
| 44 | `item44_factor_template_code.cpp` | 不同 N 的矩阵复用只依赖 T 的核心实现 |
| 45 | `item45_member_template.cpp` | Derived view 转 Base view，反向转换在编译期拒绝 |
| 46 | `item46_hidden_friend.cpp` | `half * 2` 与 `2 * half` 均通过 hidden friend 工作 |
| 47 | `item47_traits.cpp` | vector/list 迭代器通过标签选择不同实现 |
| 48 | `item48_metaprogramming.cpp` | 递归 TMP、constexpr 和 if constexpr |

语言细节可进一步核对标准草案的 [依赖名称](https://eel.is/c++draft/temp.dep)、[模板实参推导](https://eel.is/c++draft/temp.deduct) 和 [约束与 concepts](https://eel.is/c++draft/temp.constr) 章节。

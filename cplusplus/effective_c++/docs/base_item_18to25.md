# Effective C++：条款 18～25 学习笔记

> 对应《Effective C++》（第三版）第四章 **Designs and Declarations（设计与声明）**。
>
> 这一章讨论的不是“怎样让代码能编译”，而是“怎样让接口表达正确意图，使正确用法自然、错误用法尽早失败”。一个好接口会让调用者几乎不必猜测资源所有权、参数单位、可否隐式转换、返回值是否存活，以及异常发生时对象会处于什么状态。

---

## 本章路线图：从接口到实现边界

| 条款 | 一句话记忆 | 主要风险 |
| --- | --- | --- |
| 18 | 让接口容易被正确使用，难以被误用 | 单位、范围、构造顺序和返回值被误解 |
| 19 | 设计 class 如同设计一种新 type | 没有先定义值语义、关系和不变量 |
| 20 | 对大对象用 `const T&` 传递 | 无意复制对象、切片、性能损耗 |
| 21 | 该返回对象时不要返回引用 | 返回局部对象/临时对象的悬空引用 |
| 22 | 数据成员保持 private | 外部代码绕过不变量、破坏封装 |
| 23 | 优先 non-member、non-friend 函数 | 成员函数过多、权限和耦合扩大 |
| 24 | 需要双侧隐式转换时用 non-member | `int * Rational` 等对称表达式失败 |
| 25 | 考虑提供不抛异常的 `swap` | 交换昂贵、泛型算法难提供异常安全 |

### 先区分四个常被混淆的概念

```text
接口（interface） ：调用者能写什么、能观察什么
实现（implementation）：类内部如何保存和完成工作
不变量（invariant）  ：对象每个有效时刻都必须成立的条件
抽象（abstraction） ：调用者只需理解“做什么”，不必依赖“怎么做”
```

例如银行账户的接口可以是 `deposit`、`withdraw`、`balance`；余额绝不能为负可能是不变量；具体用整数分还是数据库字段保存则是实现。若把 `balance_` 公开，外部代码可直接写出负数，接口就失去了维护不变量的能力。

### 本篇缩写与术语速查表

| 术语/缩写 | 全称或含义 | 在本篇中为什么重要 |
| --- | --- | --- |
| API | Application Programming Interface，应用程序编程接口 | 一个类/库向调用者公开的函数、类型和行为约定。 |
| RAII | Resource Acquisition Is Initialization，资源获取即初始化 | 用对象生命周期自动管理资源；详见条款 13。 |
| RVO | Return Value Optimization，返回值优化 | 编译器直接在调用者的返回值位置构造对象，减少按值返回的复制。 |
| NRVO | Named Return Value Optimization，具名返回值优化 | RVO 的常见形式：`return result;` 时直接构造调用者的结果。 |
| ADL | Argument-Dependent Lookup，实参依赖查找 | 调用未限定的 `swap(a, b)` 时，编译器会到参数类型所在 namespace 寻找匹配 swap。 |
| Pimpl | Pointer to Implementation，指向实现的指针 | 把大型私有实现藏到 `Impl` 对象后，只在公开类中保存一个指针。 |
| `Impl` | implementation 的常用缩写 | Pimpl 模式中保存真实私有数据/实现细节的类型名。 |
| LHS / RHS | Left-Hand Side / Right-Hand Side，左/右操作数 | 解释 `lhs = rhs`、`2 * half` 等运算符两侧的对象。 |
| ABI | Application Binary Interface，二进制应用接口 | 库升级时对象布局、符号和调用约定的二进制兼容性约束；Pimpl 常用来降低布局变化影响。 |
| `std` | C++ standard library 的命名空间 | `std::string`、`std::swap`、智能指针等标准库组件均位于其中。 |

`noexcept`、`explicit`、`const`、`private`、`namespace` 等是 C++ 关键字或语言术语，不是缩写；文中会在首次关键位置说明其语义。

---

## 条款 18：让接口容易被正确使用，不易被误用

### 核心结论

接口设计的目标不止是提供功能，还要把错误尽可能前移到编译期或对象创建时。不要要求每个调用者都记住隐藏规则。

### 1. 让类型表达单位和含义

最危险的接口之一是多个整数参数含义相似但单位不同：

```cpp
void schedule(int year, int month, int day, int timeout);
schedule(2026, 13, 40, 5); // 能编译，但 month/day 无效；timeout 是秒还是毫秒？
```

更好的做法是用类型和构造函数表达约束：

```cpp
enum class Month { jan = 1, feb, mar, apr, may, jun,
                   jul, aug, sep, oct, nov, dec };

class Date {
public:
    Date(int year, Month month, int day); // 构造时检查 day 合法性
};

Date birthday{2026, Month::sep, 3};
// Date wrong{2026, 13, 40}; // 编译期就难以写出错误月份
```

`enum class` 让月份成为一个独立类型，避免把任意 `int` 悄悄当月份。它不能自动验证所有业务规则（例如二月天数），但能先消灭一大类错误输入。

### 2. 用构造函数建立不变量

对象一旦构造完成，使用者应能假设它处于有效状态：

```cpp
class Percentage {
public:
    explicit Percentage(int value) {
        if (value < 0 || value > 100) throw std::out_of_range("percentage");
        value_ = value;
    }
private:
    int value_{};
};
```

这里不变量是 `0 <= value_ <= 100`。检查放在构造函数而不是每次业务函数都检查，原因是“非法对象根本无法诞生”；后续成员函数的推理更简单。

### 3. `explicit` 阻止意外单参数转换

```cpp
class Password {
public:
    explicit Password(std::string text) : text_(std::move(text)) {}
private:
    std::string text_;
};

void login(const Password& password);
// login("123456"); // explicit 时不能让字符串字面量悄悄变成 Password
login(Password{"123456"}); // 调用者明确表达转换意图
```

单参数构造函数默认可作为隐式转换通道。对数值包装、单位类型、资源句柄等，隐式转换很容易掩盖错误，因此默认考虑 `explicit`。条款 24 会讨论少数刻意允许隐式转换的场景。

### 4. 用返回类型让错误难以忽略

现代 C++ 可使用 `[[nodiscard]]` 提醒调用者处理关键结果：

```cpp
[[nodiscard]] bool save(const Document& document);

save(doc); // 编译器通常警告：忽略了是否保存成功
```

这不是强制错误处理，但能把“忘记检查”变成可见的编译警告。对错误码、锁定结果、资源获取结果尤其有用。

### 5. 工厂函数有时比构造函数更清楚

当创建方式较多、参数顺序容易混淆、或对象创建可能失败时，具名工厂能表达意图：

```cpp
auto utc = TimePoint::fromUtc(2026, 9, 3, 8, 30);
auto local = TimePoint::fromLocal(2026, 9, 3, 8, 30);
```

相比只有一个 `TimePoint(int, int, int, int, int)`，调用点能直接读出时区含义。工厂也可返回 `std::optional<T>`、`std::expected<T, E>`（C++23）或智能指针来表达“可能无法创建”。

### 接口设计检查表

1. 参数顺序、单位、所有权和可空性是否靠注释才能理解？若是，能否用类型表达？
2. 非法值是否能在构造时被拒绝？
3. 单参数构造函数是否会造成不该有的隐式转换？
4. 关键返回值是否可能被无意忽略？
5. 调用者犯错后，错误会在编译期、运行初期，还是生产环境才暴露？越早越好。

---

## 条款 19：设计 class 犹如设计 type

### 核心结论

当你定义一个 class，不是在“打包几个变量”，而是在向程序引入一种新类型。应先决定它像 `int` 那样的值、像文件句柄那样的资源、还是像多态接口那样的抽象。

### 设计一个类型前应回答的问题

| 问题 | 例：`Rational`（有理数） |
| --- | --- |
| 对象如何创建和销毁？ | 分母不能为 0；构造时约分 |
| 是否允许复制、移动、赋值？ | 值类型通常全部允许 |
| 什么是有效状态？ | 分母始终为正；分子分母互质 |
| 是否支持转换？ | `int` 是否可隐式成为 `Rational`？需按接口需要决定 |
| 支持哪些运算？ | `+`、`-`、比较、输出；每个运算的返回类型与异常保证 |
| 是否需要继承？ | 数学值类型通常不需要多态继承 |
| 谁能访问内部表示？ | 外部应只能见到分子/分母的受控只读接口 |

### 值语义、实体语义与多态语义

```text
值语义（value semantics）
  复制得到独立但相等的值；例如 string、vector、Rational。

实体/身份语义（entity semantics）
  两个对象即使字段相同也可能是不同实体；例如银行账户、数据库连接。

多态语义（polymorphic semantics）
  调用者通过基类接口操作不同派生实现；例如 Shape、Plugin。
```

把它们混在一起会造成模糊 API。例如数据库连接复制后到底是新连接、共享连接，还是禁止复制？这不是实现细节，必须成为类型设计的一部分。

### 类型不变量如何降低全局复杂度

```cpp
class Rational {
public:
    Rational(int numerator, int denominator);
    int numerator() const noexcept;
    int denominator() const noexcept;

private:
    int numerator_{};
    int denominator_{1};
};
```

若构造函数保证 `denominator_ != 0` 且规范化符号，那么加法、比较、输出函数都不必在每一处重新判断“分母会不会为 0”。不变量把检查集中到状态入口，减少所有后续代码的分支数量。

### 机器层面的补充：类型通常不等于零成本

抽象不意味着没有成本，但“用类型消除错误”通常比手写运行时补救便宜得多。小值类型按值传递可能只在寄存器或栈上复制几个字节；含动态内存的类型复制可能分配堆内存、复制缓存行。条款 20 会解释为什么参数传递需要观察类型大小和语义，而不是只记口号。

---

## 条款 20：宁以 pass-by-reference-to-const 替换 pass-by-value

### 核心结论

对于较大的对象、或需要保持动态类型的对象，优先用 `const T&` 传参：避免复制、避免切片，并承诺函数不通过该引用修改对象。小型、平凡类型则通常按值传递。

### 按值传递实际上发生了什么

```cpp
void printName(Person person); // 参数 person 是实参的一个副本
```

调用 `printName(employee)` 时，需要构造 `person`；函数结束还要析构 `person`。若 `Person` 含 `std::string`、`std::vector`、互斥锁包装或大量成员，复制可能涉及堆分配、引用计数操作、缓存读取和析构。

```cpp
void printName(const Person& person); // person 只是原对象的只读别名
```

引用通常只传递一个地址大小的值，不构造新 `Person`。`const` 让函数无法通过该参数修改原对象，并允许绑定临时对象。

### 多态对象的切片问题

```cpp
class Window {
public:
    virtual void display() const;
};
class SpecialWindow : public Window {
public:
    void display() const override;
};

void showByValue(Window window) { window.display(); }
void showByReference(const Window& window) { window.display(); }
```

传入 `SpecialWindow` 时，`showByValue` 会**复制其中的 `Window` 基类部分，派生部分被切掉（object slicing）**，调用的是 `Window::display()`；引用版本保持真实对象，动态派发仍会调用 `SpecialWindow::display()`。

### 不是所有东西都该传 `const T&`

| 类型/用途 | 常见选择 | 原因 |
| --- | --- | --- |
| `int`、`double`、枚举、指针 | 按值 | 复制极小，引用反而增加一次间接访问 |
| 小型平凡 struct | 通常按值 | 寄存器传递常更简单；用基准测试确认热点 |
| `std::string`、大型容器、复杂对象 | `const T&` | 避免复制且只读 |
| 函数必须获得并保存一份数据 | 按值 | 可移动到成员，所有权语义清楚 |
| 多态基类 | `const Base&` 或智能指针 | 保留动态类型，避免切片 |

按值接收后再 move 的常见现代模式：

```cpp
class User {
public:
    explicit User(std::string name) : name_(std::move(name)) {}
private:
    std::string name_;
};
```

调用者给左值时会复制一次，给右值时会移动；类获得自己的所有权。这与“函数只是读取、不保存”的 `const std::string&` 是不同需求。

### 低层直觉与边界

引用不是对象本身；实现上通常以地址传递，访问成员可能多一次间接寻址。它也会引入别名：函数不知道其他地方是否同时修改对象。因此不要把 `const T&` 当作无条件性能优化；先看语义，再看对象大小和测量结果。

---

## 条款 21：必须返回对象时，别妄想返回引用

### 核心结论

当函数计算出一个新结果时，应按值返回对象。不要为了避免复制返回指向局部对象、临时对象或无法保证存活期对象的引用；现代 C++ 的返回值优化和移动语义通常会消除或降低复制成本。

### 最典型的悬空引用

```cpp
const Rational& operator*(const Rational& left, const Rational& right) {
    Rational result(left.numerator() * right.numerator(),
                    left.denominator() * right.denominator());
    return result; // 错误：函数返回时 result 已析构，引用悬空
}
```

`result` 是局部自动对象。离开函数时它的生命周期结束；调用者拿到的引用指向一块已经不再是 `Rational` 对象的内存。之后读取可能“看似正常”、打印垃圾值或随机崩溃，都是未定义行为。

### 原书三种“为了返回引用而修补”的写法

这条款最容易误解的地方是：问题不只是“局部变量在 stack（栈）上”，而是**函数返回后，调用者拿到的引用是否仍指向一个活着、独立、由谁负责的对象**。原书依次讨论了几种看似能让对象活得更久的做法，它们各有不同问题。

#### 情况一：返回局部自动对象的引用——悬空引用

```cpp
const Rational& multiplyBad1(const Rational& left, const Rational& right) {
    Rational result(left.numerator() * right.numerator(),
                    left.denominator() * right.denominator()); // 局部自动对象
    return result;                   // 错误
}
```

通常实现中，这种局部对象的相关数据位于当前函数调用的栈帧内；`multiplyBad1` 返回时，其栈帧退出，`result` 的析构函数已经执行。即使之后那片栈内存暂时还没有被别的函数覆盖，`result` 的**对象生命周期已结束**，不能再通过引用访问。

更严格地说，C++ 标准描述的是“自动存储期（automatic storage duration）”，不是必须使用某种 CPU 栈布局。绝大多数平台会用栈实现它，但正确性依据始终是对象生命周期，而不是“那块内存看起来还在”。

```text
进入 multiplyBad1：创建 result
return result 的引用
离开 multiplyBad1：析构 result，函数栈帧退出
调用者使用引用：引用指向已死亡对象 → 未定义行为
```

#### 情况二：用 `new` 创建对象再返回引用——不悬空，但无人负责 delete

初学者常想到：既然局部对象会销毁，那就放到 heap（堆）上。

```cpp
const Rational& multiplyBad2(const Rational& left, const Rational& right) {
    Rational* result = new Rational(left.numerator() * right.numerator(),
                                    left.denominator() * right.denominator());
    return *result; // 引用指向仍活着的堆对象
}
```

这避免了情况一的“函数返回立即析构”，但制造了更隐蔽的问题：调用者得到的是 `const Rational&`，接口没有告诉它“你拥有这个对象，必须 delete”。

```cpp
const Rational& answer = multiplyBad2(a, b);
// delete &answer; // 调用者通常不会知道或不敢这样写
```

于是 `new Rational` 对应的资源永远没有释放路径，造成内存泄漏。重复调用乘法会不断分配堆内存；长期运行的服务会逐渐增大内存占用。更糟的是，调用者即使猜到需要 `delete &answer`，这种接口也非常反直觉，容易重复释放或在异常路径漏释放。

同样要严格表述：C++ 标准称其为**动态存储期（dynamic storage duration）**；常见实现会从堆分配，但语言保证的是“对象持续存在到 `delete`”，而不是特定堆地址布局。

#### 情况三：返回 static 对象的引用——不泄漏，但结果互相覆盖

```cpp
const Rational& multiplyBad3(const Rational& left, const Rational& right) {
    static Rational result(0, 1);
    result = Rational(left.numerator() * right.numerator(),
                      left.denominator() * right.denominator());
    return result;
}
```

static 局部对象拥有静态存储期：第一次执行到声明处时构造，通常直到程序结束才析构，所以引用不会因函数返回立刻悬空，也不需要每次 `new/delete`。

但整个程序只有**同一份** `result`：

```cpp
const Rational& first = multiplyBad3(a, b);
const Rational& second = multiplyBad3(c, d);
// first 和 second 都引用同一个 result；第二次调用已覆盖 first 看到的值。
```

这会使嵌套表达式、保存旧结果、递归调用都得到错误值；多线程同时调用还会产生数据竞争。C++11 保证 static 局部对象的**首次构造**线程安全，但不保证之后对 `result` 的并发读写安全。

### 三种存储期的对比

| 对象来源 | 生命周期 | 返回引用的问题 |
| --- | --- | --- |
| 函数局部普通变量 | 离开作用域时结束 | 函数返回后立刻悬空 |
| `new` 创建的对象 | 到对应 `delete` 为止 | 返回引用没有表达谁负责 delete，容易泄漏 |
| `static` 局部对象 | 首次初始化后直到程序结束 | 所有调用共享同一个结果，覆盖/并发问题 |
| 按值返回的结果对象 | 调用者获得自己的结果对象 | 正确默认；编译器可消除复制或移动 |

### 现代 C++ 的正确选择

对数学乘法、字符串拼接、容器算法结果等“计算出一个新值”的函数，直接返回值：

```cpp
Rational multiply(const Rational& left, const Rational& right) {
    Rational result(left.numerator() * right.numerator(),
                    left.denominator() * right.denominator());
    return result;
}

Rational answer = multiply(a, b);
```

这不是把 `result` 的引用交给调用者。编译器会把返回值构造在调用者结果对象的位置（NRVO），或在不能消除时移动它；调用者拥有自己的 `answer`，离开作用域时自动析构。没有悬空、没有“谁 delete”、没有共享结果覆盖。

只有当“结果本来就应由动态分配、并且所有权需要转移”时，才返回拥有者类型：

```cpp
std::unique_ptr<LargeObject> createLargeObject() {
    return std::make_unique<LargeObject>();
}

auto object = createLargeObject(); // object 明确接管所有权；自动释放
```

不要返回裸指针/裸引用来暗示所有权转移。`std::unique_ptr` 把“唯一负责释放”写进类型；多个真正共享的拥有者才考虑 `std::shared_ptr`。而对普通值计算，动态分配通常根本不需要。

正确写法：

```cpp
Rational operator*(const Rational& left, const Rational& right) {
    return Rational(left.numerator() * right.numerator(),
                    left.denominator() * right.denominator());
}
```

### 为什么按值返回通常不慢

现代编译器可使用：

- **RVO/NRVO**：直接在调用者提供的返回值存储位置构造对象；
- **移动构造**：若不能消除复制，资源可从临时对象转移而非深拷贝；
- **C++17 保证复制消除**：某些 prvalue 返回场景不再需要可访问的复制/移动构造。

因此“返回引用肯定更快”是过时且危险的直觉。先保证生命周期正确，再让编译器优化。

### 何时返回引用是正确的

若函数返回的是一个**仍由调用者或某个明确对象拥有、且寿命足够长**的现有对象，返回引用合理：

```cpp
class Dictionary {
public:
    const std::string& name() const noexcept { return name_; }
private:
    std::string name_;
};
```

调用者必须保证 `Dictionary` 在引用使用期间仍存活，且相关操作没有让引用失效。对容器元素、`string_view`、`span` 也是同一条生命周期规则。

### 静态临时对象也通常不是答案

把局部变量改成 `static` 可以避免悬空，但会造成所有调用共享同一对象：多线程数据竞争、嵌套调用互相覆盖、上次结果污染下次结果。除非接口明确返回全局共享状态，否则不要用它修补返回引用问题；上面的“三种修补写法”已展示其具体原因。

---

## 条款 22：将数据成员声明为 private

### 核心结论

数据成员 private 不是为了“故意隐藏”，而是为了让类能够独占维护不变量、验证输入并在未来改变表示方式。即使读操作，也优先通过受控接口暴露。

### public 数据如何破坏不变量

```cpp
struct BadAccount {
    long long balanceCents; // 外部可直接写任意值
};

BadAccount account{1000};
account.balanceCents = -999999; // 无法阻止
```

改成 private：

```cpp
class Account {
public:
    explicit Account(long long initialCents);
    void deposit(long long cents);
    bool withdraw(long long cents);
    long long balanceCents() const noexcept;

private:
    long long balanceCents_{};
};
```

现在只有 `Account` 的成员函数能修改余额。它可以集中检查“金额非负”“余额足够”“是否溢出”，并把内部表示从整数分改为十进制类型、加密字段或远端缓存，而不强迫调用者修改所有访问代码。

### public/protected 数据为什么都不好

`protected` 仍允许派生类直接依赖内部表示。以后基类想替换成员类型、延迟计算或加锁时，所有派生类都可能被破坏。除非你刻意把数据表示作为派生类扩展点公开，否则数据成员通常应 private；派生类通过 protected 成员函数访问受控行为。

### getter 不等于随意暴露引用

```cpp
const std::string& name() const noexcept { return name_; } // 常见，只读借用
```

它避免复制，但返回引用的寿命绑定到对象。若返回非 const 引用：

```cpp
std::string& name() { return name_; } // 调用者能绕过验证任意修改
```

就等于把修改权重新交给外部。是否提供非 const getter，应由不变量和接口语义决定。

---

## 条款 23：宁以 non-member、non-friend 替换 member 函数

### 核心结论

一个函数能不成为成员、也不需要 friend，就应优先作为普通 non-member、non-friend 函数。它只依赖类的 public 接口，减少对内部表示的访问权限，并让相关操作按功能分组到 namespace 中。

> 一个函数如果不访问类的`private/protected`，没必要给他这种权限
> member 函数隐含的第一个参数是`this`，non-member && non-friend函数只能用类暴漏的public接口
> friend 函数可以访问private成员，但没有`this`

### 成员函数并不比普通函数“更封装”

假设浏览器类已有公开基础操作：

```cpp
class WebBrowser {
public:
    void clearCache();
    void clearHistory();
    void removeCookies();
};
```

“清除全部浏览数据”可以写成成员：

```cpp
class WebBrowser {
public:
    void clearEverything();
};
```

也可以写成普通函数：

```cpp
void clearEverything(WebBrowser& browser) {
    browser.clearCache();
    browser.clearHistory();
    browser.removeCookies();
}
```

后者不访问 private 数据，也不需要 friend；它只依赖公开接口。若未来 `WebBrowser` 内部改变，只要三个公开函数的契约不变，`clearEverything` 不必改变。

### 为什么 non-member 更利于扩展

成员函数只能写进类定义；普通函数可按功能放进 namespace 的不同头文件：

```text
webbrowser/core.h       ：WebBrowser 基本类型
webbrowser/bookmarks.h  ：书签相关非成员函数
webbrowser/cookies.h    ：cookie 相关非成员函数
```

调用者只包含所需功能头文件，减少编译依赖和接口膨胀。C++ 的 namespace 是比“巨型类”更适合组织独立功能的单位。

### “封装”应该按权限衡量

封装不是“函数放在 class 大括号里”的程度，而是“有多少代码能访问实现细节”。

```text
成员函数       ：能访问 private
friend 函数    ：能访问 private
普通非成员函数 ：只能访问 public
```

因此普通非成员函数的权限最小。最小权限原则使内部表示更容易替换，也减少错误修改私有状态的机会。

---

## 条款 24：若所有参数都需要类型转换，请为此采用 non-member 函数

### 核心结论

成员运算符的左操作数必须已经是该类对象；因此只有右操作数能使用隐式转换。若希望 `2 * half` 与 `half * 2` 都工作，二元运算符应是 non-member。

### 成员版本为什么不对称

```cpp
class Rational {
public:
    Rational(int numerator = 0, int denominator = 1); // 故意允许 int → Rational
    Rational operator*(const Rational& rhs) const;
};

Rational half(1, 2);
half * 2; // 可行：2 可转换为 Rational，作为右参数
2 * half; // 不行：2 不是 Rational，不能调用 2.operator*(half)
```

编译器不会先把左侧的 `2` 变为 `Rational`，再寻找成员函数；成员调用要求左对象已经属于该类。

### non-member 版本如何恢复对称性

```cpp
class Rational {
public:
    Rational(int numerator = 0, int denominator = 1);
    int numerator() const noexcept;
    int denominator() const noexcept;
};

Rational operator*(const Rational& left, const Rational& right) {
    return Rational(left.numerator() * right.numerator(),
                    left.denominator() * right.denominator());
}

Rational half(1, 2);
half * 2; // 两个参数均可转换
2 * half; // 两个参数均可转换
```

这个函数不需要 friend，只要 `Rational` 提供必要的 public 观察接口。它同时满足条款 23 的“最小权限”。

### 不要为了对称性滥开隐式转换

这里允许 `int → Rational` 是数学类型的合理选择；对 `Password`、`UserId`、文件句柄、长度单位等，隐式转换通常会造成误用，应使用 `explicit`。条款 18 与条款 24 不是矛盾：先判断“这种自动转换在语义上是否自然且安全”，再决定构造函数是否 explicit。

---

## 条款 25：考虑写出一个不抛异常的 swap 函数

### 核心结论

`swap` 的语义是交换两个对象的值。对资源管理类，若能只交换内部指针/句柄而不是复制完整对象，通常更快；若 `swap` 不抛异常，泛型算法更容易实现强异常安全保证。

### 为什么 `std::swap` 有时太贵

默认 `std::swap` 可粗略理解为：

```cpp
template <class T>
void swap(T& a, T& b) {
    T temp(std::move(a));
    a = std::move(b);
    b = std::move(temp);
}
```

对只含几个整数的类没问题；但若对象内部持有很大的缓冲区、Pimpl 指针、文件资源，三次移动/赋值可能仍比直接交换内部句柄复杂。对旧式未优化类型，甚至可能发生深拷贝和分配。

### Pimpl 是什么，为什么它常与 swap 一起出现

**Pimpl = Pointer to Implementation（指向实现的指针）**。公开类不直接保存大量私有数据，而是只保存一个指向 `Impl` 的智能指针；真正的成员、第三方库头文件和复杂实现放到 `.cpp` 文件中。

```cpp
// widget.h：调用者只需看见这一小部分
class Widget {
public:
    Widget();
    ~Widget(); // 通常在 widget.cpp 中定义
    void swap(Widget& other) noexcept;

private:
    struct Impl;                  // 仅前向声明，不暴露具体字段
    std::unique_ptr<Impl> impl_;  // Widget 的公开对象通常只含一个指针大小的成员
};

// widget.cpp：真正实现可随时调整
struct Widget::Impl {
    std::vector<int> largeCache;
    std::string configuration;
    // 还可以包含复杂第三方类型
};
```

这样做有三个常见收益：

1. **编译依赖更小**：改 `Impl` 的字段时，包含 `widget.h` 的大量源文件通常不必重新编译。
2. **封装更强**：调用者看不到真实字段，也不能依赖其布局。
3. **交换更快**：两个 `Widget` 交换时只需交换两个 `unique_ptr`，不必复制/移动 `largeCache` 的全部元素。

这也是条款 25 示例中 `std::unique_ptr<Impl> impl_` 的含义：它模拟 Pimpl 的“一个轻量句柄指向重量级实现”。

注意，`std::unique_ptr<Impl>` 析构时需要知道 `Impl` 的完整定义；因此真实 Pimpl 中，`Widget` 的析构函数通常要在 `.cpp` 中、`Impl` 定义之后再实现。Pimpl 也有代价：额外一次动态分配、一次指针间接访问，以及更复杂的复制语义设计；小型简单类不必为了模式而使用它。

### 为类提供高效交换

```cpp
class Widget {
public:
    void swap(Widget& other) noexcept {
        using std::swap;
        swap(impl_, other.impl_); // 只交换内部智能指针/句柄
    }
private:
    std::unique_ptr<Impl> impl_;
};

void swap(Widget& left, Widget& right) noexcept {
    left.swap(right);
}
```

成员 `swap` 能访问内部表示；同一 namespace 的非成员 `swap` 让**实参依赖查找（ADL）**找到针对 `Widget` 的高效版本。

泛型代码推荐这样调用：

```cpp
using std::swap;
swap(left, right);
```

先引入 `std::swap` 作为通用后备，再进行未限定的 `swap(left, right)` 调用。若参数类型所在 namespace 有更匹配的 `swap`，ADL 会选它；否则使用 `std::swap`。

### `noexcept` 为什么重要

排序、容器重排、copy-and-swap 赋值等算法常需要在中途失败时回滚。若交换已完成且保证不抛异常，算法能安全地把对象恢复到旧状态；若 swap 也可能抛异常，回滚本身也可能失败，强异常安全保证更难实现。

`noexcept` 是承诺，不是优化开关：只有当所有内部交换都确实不会抛异常时才能标注。若错误标为 `noexcept` 而内部抛异常，程序会 `std::terminate()`。

> noexcept 是程序员向编译器承诺：这个函数不会让异常从函数里逃出去。
> 真逃出去违反了，就`std::terminate()`，noexcept允许写throw
> 正常try-catch只要在内部有异常没逃逸出去就没事

### 实用边界

- 不要向 `std` namespace 随意添加普通重载；对自定义类型，在自己的 namespace 定义非成员 `swap`。
- 为你自己的类定义 `std::swap` 的全特化在特定条件下可行，但通常不如“成员 swap + 同 namespace 非成员 swap + ADL”简单。
- `std::vector`、`std::string` 等标准类型已经提供高效 swap；优先直接使用。
- 只有当交换确实更高效或异常安全更重要时才自定义；普通小值类型不需要仪式化地手写 swap。

```text
                泛型调用者
                    │
                    │
             using std::swap;
             swap(a, b);
                    │
          ┌─────────┴─────────┐
          │                   │
       有专用版             没专用版
          │                   │
         ADL                  │
          ↓                   ↓
    MyLib::swap           std::swap
          │
          ↓
    a.swap(b)
          │
          ↓
  Widget::swap
          │
          ↓
  高效交换内部资源
```

---

## 条款 18～25 的逻辑链

```text
先让类型与接口表达业务约束（18、19）
          ↓
让参数传递和返回值符合对象大小、动态类型与生命周期（20、21）
          ↓
隐藏表示，控制谁能修改对象状态（22、23）
          ↓
让运算符转换规则对称且符合业务语义（24）
          ↓
为资源对象提供高效、可靠的状态交换基础（25）
```

## 初学者设计接口时的检查清单

1. 能否创建非法对象？非法参数能否由类型或构造函数拒绝？
2. 这是值类型、身份类型，还是多态接口？复制、移动、比较的语义明确吗？
3. 函数只读大对象时，是否应为 `const T&`？它是否需要保存一份数据？
4. 返回的是新计算结果还是现有对象？前者通常按值，后者才考虑引用。
5. 外部代码能否绕过成员函数直接破坏数据？
6. 某个功能真的需要 private 访问吗？能否成为 non-member non-friend？
7. 隐式转换是刻意设计的便利，还是潜在误用？
8. 资源类交换时能否只换句柄，并保证不抛异常？

---

## 配套可执行示例

示例位于 [`code/item_18to25`](/home/jason/study/ck_study/cplusplus/effective_c++/code/item_18to25)，每个 `.cpp` 可独立编译：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic item18_safe_interface.cpp -o item18
./item18
```

| 条款 | 文件 | 观察重点 |
| --- | --- | --- |
| 18 | `item18_safe_interface.cpp` | `enum class`、构造验证、`explicit`、`[[nodiscard]]`。 |
| 19 | `item19_type_design.cpp` | 有理数的规范化不变量与值语义。 |
| 20 | `item20_pass_by_const_ref.cpp` | 传值产生复制，const 引用避免复制且保留动态类型。 |
| 21 | `item21_return_by_value.cpp` | 按值返回新对象与 RVO/移动语义的安全模型。 |
| 22 | `item22_private_data.cpp` | private 数据如何集中维护账户不变量。 |
| 23 | `item23_nonmember_nonfriend.cpp` | 仅使用 public 接口的普通辅助函数。 |
| 24 | `item24_nonmember_conversion.cpp` | non-member 运算符支持两侧 `int ↔ Rational` 转换。 |
| 25 | `item25_nothrow_swap.cpp` | 成员 swap、ADL 和不抛异常的指针交换。 |

建议先运行每个程序，再故意取消其中标注为“错误”的注释，观察编译器阻止了什么。不要执行会产生未定义行为的反例；文档已说明其后果。

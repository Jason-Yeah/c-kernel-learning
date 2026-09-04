# Effective C++：条款 5～12 学习笔记

> 对应《Effective C++》（第三版）第二章 **Constructors, Destructors, and Assignment Operators（构造、析构与赋值）**。
>
> 这一章围绕同一个问题：**一个对象从出生到销毁，谁负责让它始终保持正确状态？** 初学时很容易把“复制”“赋值”“析构”当成语法细节；实际上它们决定资源是否泄漏、对象能否安全多态使用、异常发生时程序会不会直接终止。
>
> 本文以原书原则为主，补充 C++11 及之后的 `= default`、`= delete`、移动语义、智能指针和 `noexcept`。示例用于理解和手敲练习，省略了部分 `#include`；不要在尚未理解资源所有权前照搬裸指针代码到业务项目。

---

## 先建立共同模型：对象的一生与六个特殊成员函数

一个对象通常经历下面的路径：

```text
创建对象 ──→ 构造（建立不变量） ──→ 使用
                                      │
                         复制构造 ←──┤──→ 复制赋值
                         移动构造 ←──┤──→ 移动赋值
                                      │
                                  析构（释放资源）
```

其中的“资源”不只指堆内存，也可以是文件描述符、互斥锁、socket、数据库事务、线程、显卡对象等。对象的**不变量**是它在每个可观察时刻都必须成立的条件，例如：

- `std::vector` 的 `size()` 不超过 `capacity()`；
- 文件封装对象要么持有一个有效句柄，要么明确处于“未打开”状态；
- 智能指针要么为空，要么唯一/共享地管理一个有效对象；
- `Employee` 的薪资、编号等成员均已初始化并满足业务规则。

### 六个特殊成员函数分别是什么

| 函数 | 触发时机 | 它处理的是 |
| --- | --- | --- |
| 默认构造函数 | `Widget w;` | 从无到有建立对象 |
| 析构函数 | 离开作用域、`delete`、容器销毁元素 | 结束对象生命周期、释放资源 |
| 拷贝构造函数 | `Widget b(a);` | 用已有对象**创建新对象** |
| 拷贝赋值运算符 | `b = a;`，且 `b` 已存在 | 用已有对象**覆盖另一个已存在对象** |
| 移动构造函数（C++11） | `Widget b(std::move(a));` | 从将要放弃的对象接管资源以创建新对象 |
| 移动赋值运算符（C++11） | `b = std::move(a);` | 从将要放弃的对象接管资源以覆盖已有对象 |

最重要的区别是：**构造函数面对的是尚不存在的对象；赋值运算符面对的是已经存在、可能已经持有资源的对象。** 所以后者必须先考虑旧资源怎么办、自我赋值怎么办、异常中途发生怎么办。

现代 C++ 的优先级是：如果类不直接管理资源，就尽量让编译器生成这些函数（Rule of Zero，零法则）；若必须自己管理资源，就把复制、移动、析构视为一个整体设计（Rule of Five，五法则）。

---

## 条款 5：了解 C++ 默默编写并调用哪些函数

### 核心结论

当你没有声明某些特殊成员函数时，编译器可能会隐式声明、定义并调用它们。它通常只是对每个基类子对象和成员逐一执行相应操作，**并不知道你的业务语义或资源所有权意图**。

### 编译器默认会做什么

对一个简单类：

```cpp
class Empty {};
```

编译器可以让它表现得近似拥有：

```cpp
class Empty {
public:
    Empty() = default;
    Empty(const Empty&) = default;
    Empty(Empty&&) = default;
    Empty& operator=(const Empty&) = default;
    Empty& operator=(Empty&&) = default;
    ~Empty() = default;
};
```

这只是帮助理解，不要把它当作所有语言版本、所有类都必然生成的逐字等价代码。是否隐式生成、是否被定义为 deleted（删除）、是否抑制移动操作，取决于你已声明的构造/析构/复制/移动函数及成员类型。

编译器生成的复制行为大致是**逐成员复制（memberwise copy）**：

```cpp
class Person {
public:
    std::string name;
    int age{};
};

Person a{"Ada", 30};
Person b = a; // b.name 复制 a.name；b.age 复制 a.age
```

对于 `std::string`、`std::vector` 等值类型，这通常正是想要的；它们自己知道如何复制内部资源。

### “能生成”不等于“语义正确”

看一个旧式、直接拥有堆对象的类：

```cpp
class BadBuffer {
public:
    explicit BadBuffer(std::size_t size) : data_(new char[size]) {}
    ~BadBuffer() { delete[] data_; }

private:
    char* data_;
};
```

若写 `BadBuffer b = a;`，编译器默认复制只会复制指针地址：

```text
a.data_ ─┐
         ├──→ 同一块堆内存
b.data_ ─┘
```

两个对象都会在析构时 `delete[]` 同一地址，造成 double free（重复释放）。这就是为什么“编译器替你生成了拷贝构造函数”并不等于“类可安全复制”。

现代项目应优先把所有权放入成员类型：

```cpp
class Buffer {
public:
    explicit Buffer(std::size_t size) : data_(size) {}

private:
    std::vector<char> data_; // vector 管理内存，复制语义正确
};
```

此时可直接使用 `= default` 或什么也不写，编译器生成的复制/移动/析构都会委托给 `std::vector`，这正是 Rule of Zero。

#### Rule of Zero、Three、Five：不是死记规则，而是资源责任边界

| 名称 | 适用时代/场景 | 应怎样理解 |
| --- | --- | --- |
| Rule of Zero（零法则） | 现代 C++ 的首选 | 类自己不写资源释放逻辑；资源交给 `std::string`、容器、智能指针、锁守卫等成员，特殊成员函数也交给编译器。 |
| Rule of Three（三法则） | C++11 以前或旧代码 | 若手写析构、拷贝构造、拷贝赋值中的任意一个，通常也必须审视另外两个，因为你很可能在直接管理资源。 |
| Rule of Five（五法则） | C++11 以后 | 在“三个”之外还要审视移动构造与移动赋值；资源能否安全转移、转移后源对象是什么状态，都要明确。 |

这些规则不是说“写了析构函数就机械地补五个函数”。真正的含义是：一旦一个类亲自承担资源所有权，复制、赋值、移动、销毁这几个入口必须有一致语义。最省心的实现通常是重构成员类型，回到 Rule of Zero。

### 哪些情况会让默认操作不可用

编译器不会“猜”出一个合理实现时，常会把隐式函数定义为 deleted。常见原因：

| 成员/基类情况 | 常见后果 | 为什么 |
| --- | --- | --- |
| 含 `const` 数据成员 | 拷贝赋值常被删除 | 已存在对象的 const 成员不能重新赋值 |
| 含**引用**数据成员 | 拷贝赋值常被删除 | 引用一旦绑定不能改绑到另一个对象 |
| 成员是 `std::unique_ptr<T>` | 拷贝构造、拷贝赋值被删除 | 唯一所有权不能隐式复制 |
| 成员/基类本身不可复制或不可移动 | 对应操作被删除 | 外层类不能绕过内层限制 |
| 用户声明了某些复制、移动、析构函数 | 其他隐式函数的生成规则会改变 | 防止编译器生成与你的手写语义冲突的操作 |

例如：

```cpp
class Connection {
public:
    explicit Connection(int id) : id_(id) {}

private:
    const int id_;
};

// Connection a{1};
// Connection b{2};
// b = a; // 通常不能编译：b.id_ 不能被重新赋值
```

这段代码要分成“创建”和“赋值”两个时刻看。

```text
Connection a{1};  → 创建 a，同时把 a.id_ 初始化为 1
Connection b{2};  → 创建 b，同时把 b.id_ 初始化为 2
b = a;            → b 已经存在；现在想让 b 变得和 a 一样
```

`id_` 的类型是 `const int`，意思是：**它只能在所属对象创建时初始化一次，之后不能再修改。** 所以构造函数必须使用成员初始化列表：

```cpp
explicit Connection(int id) : id_(id) {}
//                              ^^^^ 创建 Connection 时，把 id_ 的唯一一次初值设为 id
```

如果改在构造函数体内赋值也不行：

```cpp
// 错误示意
Connection(int id) {
    id_ = id; // id_ 在进入函数体前就必须完成初始化，且 const 不能再被赋值
}
```

现在看 `b = a`。编译器若生成普通的逐成员拷贝赋值，逻辑近似于：

```cpp
// 编译器想做的概念步骤
b.id_ = a.id_; // 想把 b 原来的 2 改成 a 的 1
```

但 `b.id_` 已经在 `Connection b{2}` 时初始化为 2，是 const，不能修改。因此编译器找不到合法的默认拷贝赋值实现，会把 `Connection::operator=(const Connection&)` 定义为 deleted；`b = a` 就不能编译。

这和**拷贝构造**不同：

```cpp
Connection c = a;
```

这里的 `c` 还不存在，拷贝构造可以在创建 `c` 的过程中把 `c.id_` 初始化为 `a.id_`（值为 1），所以 const 成员并不天然禁止拷贝构造；它主要妨碍“给一个已经存在的对象重新赋值”。

`explicit` 与这个问题无关。它只是禁止 `int` 自动隐式转换为 `Connection`；`Connection a{1};` 这种直接构造仍然允许。

业务上也应问：`id_` 是否代表连接对象不可改变的身份？若是，禁止 `b = a` 往往是正确设计。理论上可以手写赋值运算符，只复制其他可变成员而保留 `b.id_`，但这会让 `b = a` 后的 b 并不完全等于 a，容易违反使用者对赋值的预期；除非业务语义非常明确，否则更建议显式 `= delete` 拒绝赋值。

### 初学者的判断顺序

当你写一个新类时，按下面顺序问自己：

1. 成员是否都是值语义、可正确copy的类型（`std::string`、`std::vector`、普通数字）？若是，优先什么都不写。
2. 类是否拥有唯一资源（`std::unique_ptr`、文件句柄、线程）？若是，通常禁止copy，明确设计**移动**或 RAII 包装。
3. 类是否需要“copy后仍指向同一资源”还是“copy出独立资源”？前者需要共享所有权或引用语义，后者需要深拷贝；两者都不能让编译器猜。
4. 是否真的需要自定义析构函数？若答案是“为了 `delete` 一个裸指针”，先考虑能否改成标准 RAII 成员。

### 与后续条款的联系

条款 5 说明编译器会尝试提供默认行为；条款 6 说明不想要时应明确拒绝；条款 10～12 说明你一旦自己写赋值/复制，就必须符合赋值协议、处理自我赋值，并复制完整的对象状态。

---

## 条款 6：若不想使用编译器自动生成的函数，就该明确拒绝

### 核心结论

当“复制这个对象”没有合理含义或风险很高时，不要依赖“使用者小心一点”。应让不合法代码在**编译期报错**。

### 现代写法：`= delete`

```cpp
class NonCopyableConnection {
public:
    NonCopyableConnection() = default;

    NonCopyableConnection(const NonCopyableConnection&) = delete;
    NonCopyableConnection& operator=(const NonCopyableConnection&) = delete;
};
```

此后以下代码会在编译期失败：

```cpp
NonCopyableConnection a;
// NonCopyableConnection b = a; // 错误：拷贝构造被删除
// a = b;                       // 错误：拷贝赋值被删除
```

`= delete` 不是运行时检查；它是类型接口的一部分。编译器会在调用点明确告诉使用者“这个操作不存在”。

### 原书的旧式写法为什么有效

原书写作时没有 `= delete`，通常把拷贝构造和拷贝赋值声明为 private，且不提供定义：

```cpp
class OldNonCopyable {
private:
    OldNonCopyable(const OldNonCopyable&);
    OldNonCopyable& operator=(const OldNonCopyable&);
};
```

类外代码无法访问 private 成员，因此会在编译期失败；即使成员或友元意外调用，因没有定义通常会在链接期失败。它的缺点是错误更晚、表达意图不够直接，且需要额外处理声明可见性。现代 C++ 直接用 `= delete`。

### 不可复制不代表不可移动

资源唯一所有权常适合“不可复制、可移动”：

```cpp
class FileOwner {
public:
    FileOwner() = default;
    FileOwner(const FileOwner&) = delete;
    FileOwner& operator=(const FileOwner&) = delete;

    FileOwner(FileOwner&&) noexcept = default;
    FileOwner& operator=(FileOwner&&) noexcept = default;

private:
    struct FileCloser {
        void operator()(std::FILE* file) const noexcept {
            if (file != nullptr) std::fclose(file);
        }
    };
    std::unique_ptr<std::FILE, FileCloser> file_;
};
```

这里的核心不是记住语法，而是所有权：复制会让两个对象以为自己都负责关闭同一个文件，移动则是把“负责关闭”的责任从旧对象交给新对象。移动后对象仍必须处于可析构、可赋值的有效状态，但其具体值通常不应依赖。上例需包含 `<cstdio>` 与 `<memory>`；实际优先使用项目已有的文件 RAII 封装。

### 什么时候不该禁止复制

不要因为“对象很大”就删除复制。复制是否存在首先是**语义**问题，不是性能问题。例如 `std::string`、配置快照、不可变值对象通常应可复制；若复制成本高，再通过按引用传参、移动、共享不可变数据或专门 API 解决性能。

### 常见扩展：禁止的不只有复制

`= delete` 也能禁止不希望发生的隐式转换或错误重载：

```cpp
void log(int);
void log(bool) = delete; // 防止把 bool 误当整数日志级别
```

这不是本条的重点，但能说明 `= delete` 是“把不允许的接口写进类型系统”的工具。

---

## 条款 7：为多态基类声明 virtual 析构函数

### 核心结论

若一个类要被当作多态基类，并可能通过基类指针删除派生对象，它必须有 **public virtual 析构函数**。否则行为未定义，派生类资源可能没有释放。

### 问题如何发生

```cpp
class Base {
public:
    ~Base() = default; // 非 virtual
};

class Derived : public Base {
public:
    ~Derived() { /* 关闭文件、释放资源等 */ }
};

Base* p = new Derived;
delete p; // 错误：通过 Base* 删除 Derived，行为未定义
```

`delete p` 根据指针的**静态类型**（这里是 `Base*`）决定析构入口。非 virtual 析构函数不会进行动态派发，因而无法保证 `Derived::~Derived()` 被调用。即使某个平台某次运行“好像调用了”，也不能依赖。

正确写法：

```cpp
class Base {
public:
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    ~Derived() override = default;
};

std::unique_ptr<Base> p = std::make_unique<Derived>();
// 离开作用域时：Derived 析构 → Base 析构
```

析构函数实际调用顺序是“最派生类先、基类后”，因为派生类部分可能依赖基类部分仍存在。

### 什么是“多态基类”

多态基类通常通过 `virtual` 函数提供运行时动态派发：

```cpp
class Shape {
public:
    virtual double area() const = 0;
    virtual ~Shape() = default;
};
```

`Shape*` 或 `std::unique_ptr<Shape>` 可以实际指向 `Circle`、`Rectangle` 等派生对象，并在调用 `area()` 时根据真实对象类型选择实现。既然它可以以基类形式使用，就很可能也会以基类形式被销毁，因此析构函数必须是 virtual。

### 不要把所有析构函数都写成 virtual

virtual 有成本：对象通常需要额外的虚函数表指针（具体布局由 ABI/编译器决定，标准不保证），析构调用也需要动态派发。更重要的是，它会暗示“此类适合作为多态基类”。

像 `std::string`、`std::vector`、数学坐标等值类型通常不是为继承多态设计的，不需要 virtual 析构函数。对一个不应被继承的普通类，虚析构函数反而扩大了接口和对象开销。

还有一种刻意设计：基类析构函数是 **protected 且非 virtual**。这表示外部代码不能通过 `Base*` 执行 `delete`，适合某些不允许多态删除的接口；但它需要非常明确的生命周期约束。初学阶段的简单规则是：**公开继承并用于多态删除 → public virtual 析构；不用于多态 → 通常不要 virtual 析构。**

### 常见误区

- `std::unique_ptr<Base>` 不会自动修复非 virtual 析构问题；默认 deleter 最终仍执行 `delete Base*`。
- 只要类有一个 virtual 函数，通常就应有 virtual 析构函数；更精确地说，是只要它可能被通过基类指针销毁，就必须如此。
- 构造/析构函数能否为 pure virtual 是高级话题；实践中不要把析构函数设计成纯虚，除非理解它仍必须有函数体定义。

---

## 条款 8：别让异常逃离析构函数

### 核心结论

析构函数应尽力完成清理，但不应把异常传播到调用者。尤其在栈展开（stack unwinding）期间，第二个异常逃离析构函数会导致 `std::terminate()`，程序直接终止。

### 什么是栈展开

可以把“调用栈”想成函数调用时一层层叠起来的盒子：`main` 调用 `level1`，`level1` 调用 `level2`。当正常 `return` 时，函数也会逐层返回；而 `throw` 会跳过中间的普通语句，寻找能处理该异常的 `catch`。在跳过去的每一层里，C++ 必须先销毁已经创建的局部对象，这个“逐层退出并析构局部对象”的过程就是**栈展开**。

```cpp
struct Trace {
    explicit Trace(const char* name) : name_(name) {
        std::cout << "construct " << name_ << '\n';
    }
    ~Trace() { std::cout << "destroy " << name_ << '\n'; }
    const char* name_;
};

void level2() {
    Trace second{"second"};
    throw std::runtime_error("database failed");
    // second 不会正常执行到函数结尾，但仍会被析构。
}

void level1() {
    Trace first{"first"};
    level2();
    // 异常没有在 level1 内处理，这一行不会执行。
}

int main() {
    try {
        level1();
    } catch (const std::exception& error) {
        std::cout << "caught: " << error.what() << '\n';
    }
}
```

需要包含 `<iostream>`、`<stdexcept>`。运行顺序应是：

```text
construct first
construct second
destroy second
destroy first
caught: database failed
```

一步一步看：

1. `main` 进入 `try`，调用 `level1()`，创建 `first`。
2. `level1()` 调用 `level2()`，创建 `second`。
3. `level2()` 执行 `throw`，但当前函数没有匹配的 `catch`，因此异常向调用者 `level1()` 传播。
4. 离开 `level2()` 前，销毁该层中已经构造的自动局部对象：`second` 先析构。
5. `level1()` 也没有处理异常，异常继续向 `main()` 传播；离开 `level1()` 前，`first` 析构。
6. 到达 `main` 的匹配 `catch`，异常在此被处理。

所以，栈展开的析构顺序遵守和普通离开作用域相同的规则：**后创建的局部对象先销毁（LIFO，后进先出）**。它不是“把内存全部清零”，而是 C++ 依次调用对象析构函数，让 RAII 资源有机会释放。

栈展开主要处理的是已经构造完成、且因离开作用域而应销毁的**自动存储期局部对象**。以下对象不要混淆：

- `static` 局部对象、全局对象不会因为某次异常立刻销毁；它们通常在程序退出时才析构。
- 用 `new` 创建、但只由裸指针保存的对象，不会因指针变量离开作用域自动 `delete`，所以可能泄漏。
- `std::unique_ptr<T>`、`std::vector<T>` 等 RAII 对象本身是局部对象；栈展开会析构它们，进而自动释放其管理的堆对象/内存。
- 若构造函数中途抛异常，尚未构造完成的最外层对象不会调用其析构函数，但已经构造完成的成员和基类子对象仍会被正确销毁。

异常从 `writeDatabase()` 向外寻找处理者时，C++ 会依次销毁已经构造的局部对象；这个过程叫栈展开。若 `Transaction::~Transaction()` 又抛出另一个异常，运行时无法同时正常传播两个异常，结果通常是调用 `std::terminate()`。

```text
第一个异常：writeDatabase() 失败
        ↓ 栈展开，开始析构 tx
第二个异常：~Transaction() 又失败并逃出
        ↓
std::terminate()：程序终止
```

### 为什么析构失败很常见

关闭文件可能遇到延迟写回失败，网络连接关闭可能失败，数据库事务 rollback/commit 可能失败。问题不在于“析构不会失败”，而在于析构函数没有合适渠道安全地把失败作为普通异常返回。

### 两种常见策略

**策略 A：析构函数捕获异常，记录后终止。** 适用于继续运行会破坏重要不变量的场景。

```cpp
class Session {
public:
    ~Session() noexcept {
        try {
            close();
        } catch (...) {
            logCriticalFailure();
            std::terminate();
        }
    }
    void close(); // 可向调用者报告失败
};
```

**策略 B：析构函数捕获并吞掉异常（通常记录）。** 适用于尽力清理、失败不应影响主流程的场景。

```cpp
class Session {
public:
    ~Session() noexcept {
        try {
            close();
        } catch (...) {
            logCleanupFailure();
        }
    }
    void close();
};
```

两种策略都说明同一点：把可能失败的、需要让调用者处理的操作放在**显式成员函数**中，而不是只放在析构函数中。

```cpp
Session session;
try {
    // 使用 session
    session.close(); // 此处可以捕获、重试、向上报告错误
} catch (const std::exception& e) {
    // 根据业务决定重试、告警或回滚
}
// 即使上面提前返回/抛异常，析构函数仍作为最后一道兜底清理。
```

### `noexcept` 的现代含义

现代 C++ 中析构函数通常是 non-throwing；其异常规范会受类和成员析构函数影响。无论细节如何，把析构函数显式写为 `noexcept` 是一个清晰承诺：若异常仍试图逃出，程序会调用 `std::terminate()`。

因此不要写这种接口：

```cpp
~Session() noexcept(false); // 技术上可表达，但几乎总会把异常处理变得危险
```

“不从析构函数抛异常”不等于“忽略错误”。正确做法是让显式 `close()` / `commit()` / `flush()` 报告可恢复错误，让析构函数负责不抛异常的兜底。

---

## 条款 9：构造和析构期间不要调用 virtual 函数

### 核心结论

构造基类子对象或析构基类子对象期间，对 virtual 函数的调用不会派发到尚未构造完成或已经销毁的派生类版本。调用的版本属于**当前正在构造/析构的类**。

### 先看对象构造与析构顺序

对 `Derived : Base`：

```text
构造：Base 成员 → Base 构造函数体 → Derived 成员 → Derived 构造函数体
析构：Derived 析构函数体 → Derived 成员 → Base 析构函数体 → Base 成员
```

当 `Base` 构造函数体运行时，Derived 的成员和构造函数体都还没执行；若此时调用 Derived 的 virtual 函数，它很可能访问未初始化成员。反过来，当 Base 析构函数体运行时，Derived 部分已经销毁；调用 Derived 版本也可能访问已销毁状态。C++ 因此把派发限制为当前类，避免这种更严重的错误。

```cpp
class Base {
public:
    Base() { log(); }         // 调用 Base::log，而不是 Derived::log
    virtual ~Base() { log(); } // 同样调用 Base::log

    virtual void log() const { std::cout << "Base\n"; }
};

class Derived : public Base {
public:
    void log() const override { std::cout << "Derived\n"; }
};
```

创建 `Derived` 时，上例输出的是两次 `Base`，不是 `Derived`。若 `Base::log` 是 pure virtual，构造/析构期间调用它更可能导致运行时错误；不要这样设计。

### 正确的设计方向

**把构造所需信息作为参数交给基类构造函数。**

```cpp
class Base {
public:
    explicit Base(std::string name) : name_(std::move(name)) {
        logConstruction(name_);
    }
private:
    std::string name_;
};

class Derived : public Base {
public:
    Derived() : Base("Derived") {}
};
```

这里 Base 不必猜测派生类型，也不调用可覆写函数。需要复杂初始化时，工厂函数可先收集参数，再创建完整对象；对象创建完以后再调用 virtual 行为。

#### 实际调用栈情况

调用栈会显示如

```text
#0  Base::print (this=0x7fffffffd8a0) at item9_virtual_call_during_lifetime.cpp:18
#1  0x0000555555555299 in Base::Base (this=0x7fffffffd8a0) at item9_virtual_call_during_lifetime.cpp:9
#2  0x000055555555535b in Derived::Derived (this=0x7fffffffd8a0) at item9_virtual_call_during_lifetime.cpp:24
#3  0x00005555555551d1 in main () at item9_virtual_call_during_lifetime.cpp:33
```

语义上要求先基类后子类，但实际上编译器把构造子类当作是

```cpp
Derived::Derived()
{
    Base::Base();   // 先调用基类构造
    // 然后才执行 Derived 自己的构造函数体
}
```

因此，从函数调用的角度看是

```text
main()
  │
  └── 调用 Derived::Derived()
             │
             └── Derived 构造函数内部先调用 Base::Base()
                          │
                          └── Base::Base() 里调用 Base::print()
```

而`this`地址都一样的，本例为`=0x7fffffffd8a0`，说明Derived对象开头就是它的Base子对象

```text
Derived 对象
地址 0x7fffffffd8a0
│
▼
┌──────────────────────┐
│ Base 子对象          │ ← Base::this
│   vptr               │
│   Base members       │
├──────────────────────┤
│ Derived 自己的成员   │
└──────────────────────┘
↑
Derived::this
```

### 常见误区与边界

- 间接调用也一样危险：Base 构造函数调用普通私有函数，私有函数再调用 virtual 函数，结果仍不会派发到派生类。
- `final` 只能限制继续派生，不能让构造期间派发到尚未构造的部分。
- 构造函数中调用自身的 private/non-virtual 辅助函数通常没有问题，前提是它只访问已经初始化的 Base 状态。
- 这条讨论的是构造/析构阶段；对象完整构造后，正常 virtual 派发照常工作。

---

## 条款 10：令赋值运算符返回 `*this` 的引用

### 核心结论

赋值运算符应像内置类型赋值一样返回左侧对象自身的引用，以支持连锁赋值，并避免不必要复制。

```cpp
class Widget {
public:
    Widget& operator=(const Widget& rhs) {
        // 复制 rhs 的状态到 *this
        return *this;
    }
};
```

`this` 是指向当前对象的指针，`*this` 是当前对象本身；返回类型 `Widget&` 表示返回它的别名而非副本。

### 为什么必须返回引用

内置类型支持：

```cpp
int a = 0;
int b = 0;
int c = 7;
a = b = c; // 等价于 a = (b = c)
```

`b = c` 必须产生一个可以继续赋给 `a` 的结果。用户自定义类型遵循同样惯例：

```cpp
Widget a;
Widget b;
Widget c;
a = b = c;
```

若 `operator=` 返回 `void`，上述写法不能通过编译；若按值返回 `Widget`，链式赋值虽可能工作，却多出一次复制/移动，并与标准赋值的常见语义不一致。

### 这是一条惯例，不是宇宙定律

本条适用于赋值运算符，包括 `+=`、`-=`、`*=` 等通常也返回 `T&`：

```cpp
class Number {
public:
    Number& operator+=(int delta) {
        value_ += delta;
        return *this;
    }
private:
    int value_{};
};
```

但不要为了链式调用牺牲清晰设计；少数赋值式 API 可能有不同约定。对普通 C++ 值类型，遵循 `T& operator=(...)` 是最少令人惊讶的选择。

---

## 条款 11：在 `operator=` 中处理“自我赋值”

### 核心结论

自我赋值不只写作 `w = w;`，也可能通过别名间接出现。错误的赋值实现可能先释放左侧资源，再尝试从同一资源复制，导致读取已释放内存。

### 最直观的错误模式

```cpp
class BitmapHolder {
public:
    BitmapHolder& operator=(const BitmapHolder& rhs) {
        delete bitmap_;                     // ① 先释放当前资源
        bitmap_ = new Bitmap(*rhs.bitmap_); // ② 再从 rhs 复制
        return *this;
    }
private:
    Bitmap* bitmap_{};
};
```

当执行 `holder = holder;` 时，`this` 与 `&rhs` 是同一对象。第 ① 步释放了 `bitmap_`，同时也释放了 `rhs.bitmap_`；第 ② 步解引用的是悬空指针，行为未定义。

别名也会让“看起来不像自赋值”的代码变成自赋值，例如同一容器元素通过两个引用传入函数，或两个对象共享同一底层资源。

### 方案一：身份检查（正确但不总是最佳）

```cpp
BitmapHolder& operator=(const BitmapHolder& rhs) {
    if (this == &rhs) {
        return *this;
    }

    delete bitmap_;
    bitmap_ = new Bitmap(*rhs.bitmap_);
    return *this;
}
```

它解决了直接自赋值，但仍有异常安全问题：若 `new Bitmap(...)` 抛出异常，旧资源已经删除，`bitmap_` 仍是悬空指针。

### 方案二：先复制，再替换

```cpp
BitmapHolder& operator=(const BitmapHolder& rhs) {
    Bitmap* newBitmap = new Bitmap(*rhs.bitmap_); // 可能抛异常，但旧状态未动
    delete bitmap_;
    bitmap_ = newBitmap;
    return *this;
}
```

现在即使复制失败，当前对象仍保留旧 `bitmap_`；复制成功后再释放旧资源。它同时处理自我赋值和异常安全：`rhs` 即使就是 `*this`，也会先得到一份独立副本。

### 方案三：copy-and-swap（现代常见技巧）

```cpp
class BitmapHolder {
public:
    friend void swap(BitmapHolder& left, BitmapHolder& right) noexcept {
        using std::swap;
        swap(left.bitmap_, right.bitmap_);
    }

    BitmapHolder& operator=(BitmapHolder rhs) { // 按值：先完成复制或移动
        swap(*this, rhs);
        return *this;
    } // rhs 离开作用域，析构原来属于 *this 的资源

private:
    Bitmap* bitmap_{};
};
```

调用 `a = b` 时，先用 `b` 构造参数 `rhs`；若复制失败，`a` 完全未改变。成功后交换 `a` 与 `rhs`，最后 `rhs` 析构时释放 `a` 原来的资源。这提供很强的异常安全保证，也天然处理自我赋值。

但这不是“永远最优”：按值参数可能需要额外复制/分配；`swap` 必须正确且 `noexcept`；若类有多个资源、性能严格，可能需要专门实现。更好的默认选择往往是根本不用裸拥有指针：

```cpp
class ModernBitmapHolder {
    std::unique_ptr<Bitmap> bitmap_;
    // 默认复制被禁用；默认移动正确。
};
```

若业务确实需要可复制的独占资源，通常用 `std::vector` 等值类型，或手写清晰的深拷贝语义，而非从裸指针开始。

### 与异常安全保证的关系

| 保证级别 | 发生异常后 |
| --- | --- |
| 不保证 | 对象可能损坏、资源泄漏或违反不变量 |
| 基本保证 | 对象仍有效、无资源泄漏，但状态可能改变 |
| 强保证 | 操作要么完全成功，要么对象保持原样 |

“先复制再替换”和正确的 copy-and-swap 常追求强保证。条款 11 的本质不是一定要写 `if (this == &rhs)`，而是要让赋值过程在别名和异常出现时仍维护不变量。

---

## 条款 12：复制对象时勿忘其每一个成分

### 核心结论

如果你手写拷贝构造函数或拷贝赋值运算符，必须复制对象的**所有状态**：直接成员、基类子对象，以及维持不变量所需的关联状态。编译器默认生成时会做逐成员复制；你手写后，编译器不会替你补漏。

### 最常见遗漏：忘记基类部分

```cpp
class Base {
public:
    Base() = default;
    Base(const Base&) = default;
    Base& operator=(const Base&) = default;
protected:
    std::string id_;
};

class Derived : public Base {
public:
    Derived(const Derived& rhs)
        : value_(rhs.value_) { // 错误：忘了 Base(rhs)
    }

    Derived& operator=(const Derived& rhs) {
        value_ = rhs.value_;   // 错误：忘了 Base::operator=(rhs)
        return *this;
    }

private:
    int value_{};
};
```

上例拷贝 `Derived` 后，`value_` 看似正确，但 `id_` 没有从 `rhs` 复制：拷贝构造时它会按 Base 的默认构造规则初始化（若可行），拷贝赋值时它保持旧值。这会制造“半复制对象”，业务错误往往很隐蔽。

正确写法：

```cpp
class Derived : public Base {
public:
    Derived(const Derived& rhs)
        : Base(rhs), value_(rhs.value_) {
    }

    Derived& operator=(const Derived& rhs) {
        Base::operator=(rhs);
        value_ = rhs.value_;
        return *this;
    }

private:
    int value_{};
};
```

### 复制构造与赋值不要互相调用

两者都需要复制相近状态，但对象所处阶段不同：

- 拷贝构造时，目标对象还不存在，应使用成员初始化列表建立它；
- 赋值时，目标对象已存在，必须处理旧状态、自我赋值和异常安全。

因此不要写：

```cpp
// 不推荐：构造函数内对尚未完全建立的对象调用赋值
Derived(const Derived& rhs) { *this = rhs; }

// 不推荐：赋值中“重新构造”当前对象来复用拷贝构造
Derived& operator=(const Derived& rhs) { /* this->~Derived(); new(this) Derived(rhs); */ }
```

更好的做法是让它们分别实现正确的初始化/赋值，或提取一个不依赖对象生命周期阶段的私有辅助函数。对成员全是正常值类型的类，最好的做法仍可能是直接 `= default`。

### “复制所有成分”不总是等于“复制所有缓存字节”

要复制的是对象的**逻辑状态和不变量**，不是机械复制每个可能可重新计算的缓存。例如缓存可以选择复制、清空后延迟重建，或共享；关键是目标对象必须仍表现得像源对象的正确副本，并且不暴露错误状态。

同样，若类含互斥锁，通常不能复制 mutex 本身；应设计复制后的对象拥有自己的新 mutex，并只复制受保护的数据。若类管理文件句柄或 socket，也要先定义“复制连接”在业务上究竟意味着重新打开、共享、禁止复制，还是复制配置但不复制连接。

### 与对象切片（slicing）的关系

```cpp
class Base { /* ... */ };
class Derived : public Base { /* 额外状态 ... */ };

void save(Base value); // 按值接收

Derived d;
save(d); // 只复制 Base 那一部分；Derived 额外状态被切掉
```

这不是条款 12 的“漏复制”错误，而是按值把派生对象转换为基类对象的语言行为，称为对象切片。多态对象通常应通过 `Base&`、`Base*` 或智能指针传递；若需要多态复制，通常提供 virtual `clone()`，而不是试图通过基类拷贝构造实现。

---

## 条款 5～12 的整体关系图

```text
成员/基类类型与资源所有权
             │
             ├─→ 条款 5：编译器默认操作是否符合语义？
             ├─→ 条款 6：不符合时，是否应在编译期禁止？
             ├─→ 条款 7：多态删除时，析构链是否完整？
             ├─→ 条款 8：析构清理失败时，异常是否会终止程序？
             ├─→ 条款 9：构造/析构期间是否错误依赖派生状态？
             ├─→ 条款 10：赋值接口是否符合常规语义？
             ├─→ 条款 11：赋值在自赋值和异常下是否保持不变量？
             └─→ 条款 12：手写复制时是否包含基类与所有逻辑状态？
```

## 初学者写类时的实用清单

1. 成员能否全部使用标准库值类型或 RAII 类型？能则优先 Rule of Zero，不写析构/复制/移动。
2. 若使用继承，基类是否真的是多态接口？若会通过基类删除，析构函数是否 `public virtual`？
3. 若资源不能复制，是否用 `= delete` 明确禁用复制？是否需要移动？
4. 若手写析构函数，是否保证任何路径都不让异常逃出？错误能否由显式 `close`/`commit` 报告？
5. 构造/析构函数或其间接调用链中是否出现 virtual 调用？能否改为构造参数或构造后操作？
6. 若手写 `operator=`，是否返回 `T&` 和 `*this`？自我赋值、别名、异常发生时对象还安全吗？
7. 若手写拷贝构造/赋值，是否列出了每个成员、每个基类，以及缓存/互斥锁/外部资源的明确策略？

## 与后续条款的简要关联

| 后续条款 | 为什么相关 |
| --- | --- |
| 条款 13～17：资源管理 | 本章知道何时构造/析构，后续将系统学习用对象管理资源与智能指针。 |
| 条款 29：异常安全 | 条款 8、11 的析构与赋值异常安全，会在后续发展为完整保证等级。 |
| 条款 32：public 继承 | 条款 7、9、12 都依赖正确理解基类与派生类的关系。 |
| 条款 34：如何区分接口继承和实现继承 | 条款 7 的虚析构、条款 9 的虚调用限制，都与多态接口设计直接相关。 |

建议练习时，不必一次手写“完整五法则”。先分别写一个只含 `std::string` 的值类型（什么都不写），一个含 `std::unique_ptr` 的独占资源类型（观察复制被禁止、移动可用），以及一个 `Base`/`Derived` 多态类型（观察 virtual 析构顺序）。每次都问：对象拥有什么资源？复制后谁负责释放？异常发生时对象还是否有效？

---

## 配套可执行示例

示例位于 [`code/item_5to12`](/home/jason/study/ck_study/cplusplus/effective_c++/code/item_5to12)，每个文件都可独立编译。建议一次只编译并运行一个：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic item5_compiler_generated.cpp -o item5
./item5
```

| 条款 | 文件 | 观察重点 |
| --- | --- | --- |
| 5 | `item5_compiler_generated.cpp` | 值成员的默认复制，以及 `unique_ptr` 的“不可复制、可移动”。 |
| 6 | `item6_delete_and_move.cpp` | `= delete` 拒绝复制、`std::move` 允许移动。 |
| 7 | `item7_virtual_destructor.cpp` | 经 `std::unique_ptr<Base>` 删除时，派生析构再基类析构。 |
| 8 | `item8_noexcept_destructor.cpp` | 显式 `close()` 可报告错误；析构函数只兜底且不传播异常。 |
| 9 | `item9_virtual_call_during_lifetime.cpp` | 基类构造/析构期 virtual 调用为何只到 `Base::report`。 |
| 10 | `item10_assignment_chaining.cpp` | `operator=` 返回 `*this` 以支持 `a = b = c`。 |
| 11 | `item11_self_assignment.cpp` | copy-and-swap 如何安全处理 `a = a`。 |
| 12 | `item12_copy_all_parts.cpp` | 手写复制时同时复制 Base 子对象与派生成员。 |

条款 10 的示例同时演示了 `first = second = third` 与 `Counter& sameFirst = (first += 4)`：两者都依赖运算符返回当前对象的引用。`sameFirst` 不是副本，而是 `first` 的别名，因此随后 `sameFirst += 1` 也会修改 `first`。

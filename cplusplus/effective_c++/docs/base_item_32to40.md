# Effective C++：条款 32～40 学习笔记

> 第三版第六章：Inheritance and Object-Oriented Design（继承与面向对象设计）。
>
> 本文按“概念 → 规则与原因 → 反例 → 改进 → 工程边界”讲解。默认示例使用 C++17；C++20 内容单独标明。配套程序均包含头文件与 main，可独立编译。错误用法留在注释中；可运行的反例用于观察逻辑错误，不故意触发未定义行为。
>
> 本章承接条款 7（虚析构）、9（构造/析构期间虚调用）、20（避免切片）、22（private 数据）、27（类型转换）和 31（接口隔离）。

## 阅读准备：一个派生对象里有什么

```cpp
class Base {
public:
    virtual ~Base() = default;
    virtual void run() const {}
private:
    int baseData_{};
};
class Derived : public Base {
public:
    void run() const override {}
private:
    int derivedData_{};
};
Derived d;
Base& b = d;
b.run();
```

`d` 是一个完整的 Derived 对象，其中包含一个 Base 子对象，以及 Derived 自己新增的状态。绑定 `Base& b = d` 不会复制，也不会切掉 Derived 部分；只是经由 Base 接口访问原对象。若写 `Base b = d;`，则创建独立 Base 对象，只复制基类部分，产生条款 20 的切片。

| 术语 | 含义与例子 |
| --- | --- |
| 静态类型 | 编译时根据声明知道的类型：上例 b 的声明类型是 Base&。 |
| 动态类型 | 实际完整对象的类型：b 所引用对象是 Derived。 |
| 重载 overload | 同一作用域下同名、不同参数的函数，由编译时选择。 |
| 覆盖 override | 派生类为继承来的虚函数提供实现；参数、const、引用限定等要满足覆盖规则。 |
| 隐藏 hide | 内层/派生类的同名声明影响外层/基类名字查找，不要求参数相同。 |
| LSP | Liskov Substitution Principle，里氏替换原则：派生对象应能履行基类承诺。 |
| NVI | Non-Virtual Interface，非虚接口：公开入口控制流程，内部虚函数负责变化点。 |
| Strategy | 策略模式：将可变算法交给独立对象或可调用对象。 |
| EBO/EBCO | Empty Base (Class) Optimization，空基类优化。 |
| MI | Multiple Inheritance，多重继承：一个类有多个直接基类。 |
| ABI | Application Binary Interface，二进制接口，规定某个平台的对象布局、调用约定等。 |
| vtable / vptr | 常见实现中的虚函数表 / 指向虚表的指针；不是 C++ 源码关键字。 |
| RTTI | Run-Time Type Information，运行时类型信息，支持部分 dynamic_cast/typeid 用途。 |

先记住三步：**名字查找 → 重载决议及访问检查 → 如适用，虚调用派发**。这是一种学习模型，不是完整编译器流水线；但它能解释本章绝大多数疑问。

## 条款 32：确定你的 public 继承塑模出 is-a 关系

### public 继承承诺了什么

`class Dog : public Animal` 表示 Dog 是一种 Animal；外部代码通常可以把 Dog 指针/引用转换为可访问且无歧义的 Animal 指针/引用。

语法转换允许还不够：调用者依据 Animal 的接口写出的正确代码，换成 Dog 后仍应满足原契约。因此 public 继承是“行为可替换”的承诺，不只是复用成员变量。

### 为什么“正方形是矩形”可能不适合 public 继承

设 Rectangle 承诺 `setWidth(w)` 只改宽，保持高不变。Square 为维持“宽高相等”而同时修改两者：

```cpp
void widen(Rectangle& r) {
    const int oldHeight = r.height();
    r.setWidth(10);
    // 调用者按 Rectangle 契约预期 r.height() == oldHeight。
}
```

Square 破坏了预期。即便 setter 都是 virtual、代码完全可编译，设计依然不符合替换原则。若 Square 拒绝改宽、改为抛异常，也是在强化输入限制。

因果链：

```text
基类允许独立改宽
→ 调用者据此编写代码
→ 派生类同时改高
→ 原本正确的调用者假设不成立
→ public 继承关系不合适
```

可以让 Rectangle 和 Square 都实现只读 `Shape::area()` 接口，各自管理尺寸。问题来自“可变矩形契约”，不是数学集合关系本身；不可变形状设计可能有不同结论。

### 鸟与企鹅：现实分类要服从软件接口

若 Bird 有 `fly()` 并承诺能飞，Penguin 就不适合直接继承这个契约。可把可飞能力放进 Flyable 接口，Bird 仅提供所有鸟共有的行为。不能只因为名词关系接近就继承。

现代 C++ 中，接口基类常配 public virtual 析构，派生实现写 override；这些帮助编译器检查语法，不能自动验证业务契约。配套 item32 会实际打印“改宽后高度是否仍相等”的检查结果，而不靠崩溃展示错误。

## 条款 33：避免遮掩继承而来的名称

### 为什么不同参数也会隐藏

```cpp
struct Base {
    void show(int);
    void show(double);
};
struct Derived : Base {
    void show(const char*);
};
Derived d;
// d.show(3); // 找到 Derived::show 后，不会再自动合并 Base 的同名重载。
```

查找先按名字 `show` 找声明，而不是先全局收集“哪个参数最合适”。派生作用域已找到该名字，就会影响基类查找；因此即使 Base::show(int) 看起来很合适，也不在这个调用的候选集合中。

修复：

```cpp
struct Derived : Base {
    using Base::show;
    void show(const char*);
};
```

`using Base::show` 把基类这些重载引入派生作用域，再一起进行重载选择。它不是复制函数体，也不是给函数增加 virtual 属性。这一查找机制可在 [C++ 标准草案的类成员查找规则](https://eel.is/c++draft/class.member.lookup) 中核对。

### 隐藏、覆盖、重载不能混用

```cpp
struct Base {
    virtual void work() const {}
    virtual ~Base() = default;
};
struct Wrong : Base {
    void work() {} // 缺少 const：隐藏名字，却没有覆盖 Base::work() const。
};
struct Right : Base {
    void work() const override {}
};
```

若给 Wrong::work 加 override，会在编译期发现签名错误。override 应作为日常防错工具使用。

只想暴露一个特定基类重载时，可写转发函数，例如 `void show(int x) { Base::show(x); }`。但 public 继承故意隐藏其他基类能力可能违背条款 32，先检查接口设计是否合理。

配套 item33 用合法的 `Hidden::show(double)` 展示：传入整数 3 仍调用 double 版本；加入 using 后，相同调用选择基类 int 版本。

## 条款 34：区分接口继承与实现继承

### 三种声明代表三种不同意图

| 基类声明 | 接口设计含义 | 派生类责任 |
| --- | --- | --- |
| pure virtual：`virtual void send() = 0;` | 必须具备 send 能力，不自动提供可继承的最终实现 | 具体类必须有非纯的最终覆盖版本 |
| 普通 virtual：`virtual void send();` | 有该能力，并允许沿用默认实现 | 可以覆盖，也可以直接继承 |
| non-virtual：`void check();` | 公共规则由基类固定实现 | 不应通过同名定义试图改变基类契约 |

`= 0` 是“纯虚”的声明语法，不是返回数字 0。含尚未实现的纯虚最终覆盖函数的类为抽象类，不能实例化，但可用于指针/引用。

### 默认实现为什么可能带来隐患

旧飞机 A、B 共用一套飞行控制，基类 `virtual fly()` 提供默认算法。新飞机 C 忘记覆盖 fly，代码仍可编译，却意外继承不适合它的控制逻辑。

改进是把“必须选择实现”与“可复用代码”分开：

```cpp
class Aircraft {
public:
    virtual ~Aircraft() = default;
    virtual void fly() const = 0;
protected:
    void defaultFly() const { /* 老型号通用算法 */ }
};
class ModelA : public Aircraft {
public:
    void fly() const override { defaultFly(); } // 显式选择复用
};
```

新型号若漏写 fly，就仍是抽象类，不能实例化。protected 辅助函数提供复用，纯虚接口要求开发者明确决策。

纯虚函数也可以在类外提供函数体；派生覆盖函数可通过 `Aircraft::fly()` 限定调用它。纯虚声明仍使基类保持抽象；纯虚析构函数则必须有定义。初学时上面的“纯虚入口 + 独立 protected 默认函数”更直观。

现代实践：基类首次声明用 virtual；派生覆盖写 override；确定不允许进一步覆盖才用 final。不要把 final 仅当作性能提示。

## 条款 35：考虑 virtual 函数以外的其他选择

虚函数表达“不同类型有不同实现”，但可变行为也能通过组合算法对象实现。原书讨论 NVI、函数指针、TR1 function、经典策略模式；现代对应工具包括 lambda、std::function 与模板策略。

### NVI：统一入口，开放一个变化点

```cpp
class Character {
public:
    virtual ~Character() = default;
    int health() const {
        const int result = doHealth(); // 派发到具体实现
        if (result < 0) throw std::logic_error("negative health");
        return result;
    }
private:
    virtual int doHealth() const = 0;
};
```

调用者只见 health；基类可以统一验证、记录或控制流程。派生类只实现计算。流程是：

```text
外部 health()
→ 基类前置处理
→ doHealth() 虚派发
→ 基类后置验证
→ 返回结果
```

private 只限制访问权限，不阻止派生类覆盖。因此派生类可以覆盖 private doHealth，但不能随便直接调用基类的 private 实现。注意 health 内部仍有 virtual 调用，不能因此忽略条款 9 的构造/析构期限制。

### 策略：将算法作为对象的一个成员

```cpp
using HealthAlgorithm = std::function<int(int)>;
// 保存算法：HealthAlgorithm algorithm_;
// 使用算法：return algorithm_(baseHealth_);
```

可传入普通函数或捕获 lambda。两名同类角色可以持有不同算法，无需为了“算法不同”新增派生类；还可设计运行期替换算法的接口。

| 方式 | 特点 | 代价/边界 |
| --- | --- | --- |
| 普通函数指针 | 轻量、适合无捕获函数 | 不能直接保存捕获状态 |
| std::function | 同一类型可存不同可调用对象，称为类型擦除 | 可能分配内存，调用可能间接；空对象调用抛 bad_function_call |
| 策略接口 + unique_ptr | 策略可有状态、生命周期明确 | 需要单独类与虚调用 |
| 模板策略 | 编译时选择实现，便于内联 | 不同策略产生不同类型；代码体积与编译依赖可能增加 |

lambda 用引用捕获时，std::function 不会自动延长被引用对象寿命。异步任务尤其要明确拥有关系。策略函数不天然拥有宿主 private 访问权：通过必要的参数传数据，避免为方便开放全部状态。

## 条款 36：绝不重新定义继承而来的 non-virtual 函数

### 相同对象为何调用不同函数

```cpp
struct Base { void identify() const; };
struct Derived : Base { void identify() const; };
Derived d;
Base& b = d;
d.identify(); // Derived::identify
b.identify(); // Base::identify
```

non-virtual 调用由表达式的静态类型决定。Derived 的定义不会替换 Base 的函数；两份函数仍存在。若用户以为它和虚覆盖一样“基类引用也会调用新实现”，就会写错逻辑。

非虚接口常意味着基类承诺行为固定。派生对象通过两种接口却表现不同，会削弱 public 继承的替换性。

若确实需要变化，设计阶段在基类用 virtual；若只是在固定流程中定制一步，采用 NVI。若动作是另一项功能，就用不同名字。给非虚函数的派生同名定义写 override 会报错，正好帮助发现误解。

配套 item36 同时调用非虚 identify 和虚 describe：前者随静态类型改变，后者通过两条访问路径都到 Derived。

## 条款 37：绝不重新定义继承而来的缺省参数值

### 默认参数与虚函数体有不同选择依据

```cpp
enum class Color { red, blue };
struct Shape {
    virtual ~Shape() = default;
    virtual void draw(Color c = Color::red) const = 0;
};
struct Circle : Shape {
    void draw(Color c = Color::blue) const override;
};
Circle circle;
Shape& shape = circle;
circle.draw(); // Circle::draw(blue)
shape.draw();  // Circle::draw(red)
```

`shape.draw()` 分成两件事：编译器根据 Shape 声明补入 red；虚调用再选 Circle 的函数体。默认参数不会随着动态类型一起变成 blue。

```text
调用表达式 shape.draw()
       │
       ├─ 编译时查到 Shape::draw：补参数 red
       └─ 虚派发确定实际对象 Circle：执行 Circle::draw(red)
```

默认参数不是函数类型的一部分，override 也不会检查两处默认值是否一致。规则见 [C++ 标准草案：默认实参](https://eel.is/c++draft/dcl.fct.default)。

即使派生类不写默认参数，`circle.draw()` 也不会自动继承基类的默认参数；通过 Shape 引用调用仍能使用 Shape 的默认值。

### 用 NVI 统一默认值

公开的 non-virtual `draw(Color c = Color::red)` 只定义一处默认值，在函数体中调用不带默认值的 private virtual `doDraw(Color)`。派生类只覆盖 doDraw，不再重复声明默认参数。

另一个更简单的选择是要求调用者始终显式传参。设计取舍取决于“省略参数”是否真的值得成为接口功能。

## 条款 38：通过复合塑模出 has-a 或 is-implemented-in-terms-of

复合/组合表示一个对象含有另一个对象，通常写成数据成员。本书这里的 composition 主要讨论按成员组织类型。

- 应用领域：Person 有 Address，是 has-a（拥有一个）。
- 实现领域：Set 用 vector 存储，是 is-implemented-in-terms-of（借助……实现）。

### Set 为什么不该 public 继承 vector

Set 承诺不重复，vector 允许重复。如果 Set public 继承 vector，调用者可使用继承的 push_back 绕过 Set::insert，塞入重复元素。实现复用得到了，集合契约却守不住。

```cpp
class IntSet {
public:
    bool insert(int x); // 若已存在则拒绝
private:
    std::vector<int> elements_;
};
```

调用者无法直接改 elements_，所有入口都维护唯一性。以后把 vector 换为 unordered_set，调用者仍使用 insert/contains；但复杂度、迭代顺序等若属于公开契约，变更时仍需考虑兼容。

配套 item38 用 vector 手写小集合，便于理解封装；线性查找是 O(n)，不是生产环境高性能集合的推荐实现。大数据量通常直接选标准集合容器。

“优先组合”的实际理由是：只公开业务需要的操作，内部类型变化更少传播；不产生不必要的派生到基类转换。成员自身不自动引入 vptr，具体成本取决于成员类型。

## 条款 39：明智而审慎地使用 private 继承

private 继承主要表示实现复用，不对外承诺 is-a。外部不能把派生对象隐式当作该 private 基类使用；基类的 public/protected 成员在派生类中可访问，但不会自动成为派生类的 public 接口。基类自己的 private 成员仍不可直接访问。

### 何时组合不够直接

需要覆盖已有基类 virtual 钩子，或访问它的 protected 能力时，private 继承可能更直接：

```cpp
class Timer {
public:
    virtual ~Timer() = default;
    void tick() { onTick(); }
private:
    virtual void onTick() = 0;
};
class Monitor : private Timer {
public:
    void poll() { tick(); }
private:
    void onTick() override { /* 采样 */ }
};
```

Monitor 使用 Timer 的流程，但对外不是一个 Timer。用户调用 poll，经 Timer::tick 虚派发回 Monitor::onTick。private 继承不关闭虚函数机制。

组合也能做到：Monitor 内部放一个派生自 Timer 的适配对象，让适配对象回调 Monitor。代码稍多，但能把继承细节局限在小类中。现代回调接口有时可以直接用 std::function 避免这层继承。

### 空基类优化为什么存在

空类完整对象的 sizeof 至少为 1，以满足独立对象的地址要求；空基类子对象在满足条件时可不额外占存储，称 EBO。空策略类作为基类可能节约空间。

C++20 的 `[[no_unique_address]] EmptyPolicy policy_;` 允许成员也利用类似空间复用机会。属性不是“强制 sizeof 减少”的承诺，最终布局受类型和实现影响。

不要为了省一个字节先选择 private 继承。先确定语义，再测量布局与性能。配套 item39 输出大小作为本机观察，不断言跨平台固定值。

## 条款 40：明智而审慎地使用多重继承

### 两个基类带来两种不同问题

```cpp
struct Scanner { void start(); };
struct Printer { void start(); };
struct Copier : Scanner, Printer {};
// Copier c; c.start(); // 同名来自两个分支，名字查找有歧义。
```

可显式 `c.Scanner::start()`，或在 Copier 提供自己的 start 来定义协调规则。不要指望“其中一个函数是 private，编译器就自动挑另一个”：访问权限不是通用的名字歧义消除器。

### 菱形继承：根基类到底要有几份

```text
       Device
       /    \
   Scanner Printer
       \    /
        Copier
```

普通继承时，Copier 有两份 Device 子对象：Scanner 路径一份、Printer 路径一份。若 Device 有 id，那么两个 id 可以不同；`Device* p = &copier;` 不知道选哪条路径。

若语义要求共同设备身份，让两个中间分支都 virtual 继承 Device：

```cpp
struct Scanner : virtual Device {};
struct Printer : virtual Device {};
struct Copier : Scanner, Printer {};
```

现在两条虚继承路径共享同一 Device 子对象。virtual 继承控制的是基类子对象共享，virtual 函数控制的是动态派发，它们是两套规则。共享虚基类与重复普通基类的规则见 [标准草案：多重继承](https://eel.is/c++draft/class.mi)。

### 谁初始化虚基类

最派生类负责初始化虚基类。若 `Device(int id)` 没有默认构造，构造 Copier 时需要 `Copier() : Device(42), Scanner(), Printer() {}`。Scanner/Printer 中写给 Device 的初始化在它们作为 Copier 的子对象时不负责初始化这份共享虚基类。

顺序大致为：虚基类 → 直接非虚基类（声明顺序）→ 成员（声明顺序）→ 当前构造函数体。析构反向进行。书中建议共享虚基类尽量少带状态，正是为了降低这些初始化责任的复杂度。

如果两条分支都覆盖同一共享虚基类的虚函数，最派生类可能需要再次覆盖以给出唯一最终覆盖函数。virtual 继承解决“共享几份基类”，不自动解决所有行为冲突。

### 常见底层实现与代价

多重继承对象中不同基类子对象可能位于不同偏移。Derived* 转换为第二个 Base* 时，编译器可能调整地址；不是所有基类指针都等于完整对象起始地址。

虚调用常由 vptr 找 vtable，再间接调用；多继承下还可能需要 thunk（调整 this 后再跳转的小段代码）。虚基类访问可能利用表中的偏移信息。布局、虚表数量和额外成本不能按某一次 sizeof 推广到所有平台。[Itanium C++ ABI 的虚表章节](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#vtable) 给出了 GCC/Clang 在许多平台采用的实现规范，但它不是所有编译器都必须遵循的 C++ 语言标准。

### 现代工程中的合理使用

一个类实现多个小型纯接口，例如 Readable、Writable，通常比继承多个带大量状态的实现类更容易维护。可以 public 继承业务接口，同时组合具体实现；某些适配场景也会 public 继承接口、private 继承实现类。

不要为了复用所有代码构造庞大菱形。判断标准是：基类身份是否明确、重复状态是否符合业务、初始化责任是否可解释。

## 从设计到调用：把条款串起来

```text
需要对外作为另一种类型使用？
  ├─ 是：检查 public is-a 契约（32）
  │      → 选择纯虚/普通虚/非虚承诺（34）
  │      → 区分查找、覆盖、默认参数（33、36、37）
  └─ 否：优先组合（38）
         → 算法可替换？选择 NVI/策略（35）
         → 必须覆盖底层钩子？考虑 private 继承（39）

多个基类？
  → 同名查找是否冲突？
  → 公共根基类需要一份还是多份？
  → 谁初始化共享虚基类？（40）
```

## 源码阅读与验证方式

每个程序独立包含 main，不要把九个 cpp 一起链接。进入 code/item_32to40 后，例如：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 item33_name_hiding.cpp -o /tmp/item33
/tmp/item33
```

| 条款/文件 | 观察结果 | 建议手动实验 |
| --- | --- | --- |
| item32_public_is_a.cpp | Rectangle 保持高，Square 改高，契约检查不同 | 思考只读 Shape 是否还需 setter |
| item33_name_hiding.cpp | Hidden(double)、Base(int)、Visible(double) | 取消 using 后重编译比较 |
| item34_interface_implementation.cpp | 基类检查、派生发送、主动复用默认实现 | 注释派生 send，观察抽象类错误 |
| item35_nvi_strategy.cpp | NVI 正常/拒绝负值；两个策略不同结果 | 修改 lambda 返回值 |
| item36_nonvirtual_redefinition.cpp | 非虚调用随静态类型变，虚调用不变 | 给 identify 加 override 看错误 |
| item37_default_arguments.cpp | 同一 Circle 收到 blue/red；NVI 都 red | 删除 Circle 默认值再调用 circle.draw() |
| item38_composition.cpp | 插入 7、7、9 返回 true、false、true | 尝试访问 private 数据 |
| item39_private_inheritance.cpp | 两次 poll 触发两次采样 | 启用注释中的外部 Timer 转换看错误 |
| item40_multiple_inheritance.cpp | 普通菱形两份身份；虚菱形同一地址/身份 | 改最派生类传给 Device 的值 |

文档代码片段用于讲解；完整可执行版本在 [配套代码目录](../code/item_32to40/)。示例的地址与 sizeof 随编译器而异，重点观察关系而非固定数字。

### 关键输出与调试位置

条款 37 的预期输出：

```text
Circle body, color=blue
Circle body, color=red
NVI Circle body, color=red
NVI Circle body, color=red
```

在 Circle::draw 设置断点，依次观察两次调用传入的 color：函数体相同，实参不同。再观察 SafeCircle::doDraw：两个调用都先经过继承来的 SafeShape::draw，默认值因此统一。

条款 40 的预期输出：

```text
ordinary ids: 1, 2
ordinary same base: false
virtual id: 42
virtual same base: true
read interface
write interface
```

在 main 内观察 left/right 与 sharedLeft/sharedRight：前两者指向不同 Device 子对象，后两者指向同一共享 Device。不要用 reinterpret_cast 强行把完整对象地址当作某条基类路径；让正常派生到基类转换完成必要的地址调整。

条款 39 的 C++20 扩展编译方式：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic item39_private_inheritance.cpp -o /tmp/item39
/tmp/item39
```

程序额外输出带 [[no_unique_address]] 的成员布局大小。即使某个平台没有得到预期的空间缩减，也不能由此认定程序错误；属性允许优化，实际大小仍由实现决定。

进一步核对覆盖、final、限定调用与动态类型规则，可读 [C++ 标准草案：虚函数](https://eel.is/c++draft/class.virtual)。草案会持续更新；本章可执行实验固定使用 C++17，不依赖最新草案新增语法。

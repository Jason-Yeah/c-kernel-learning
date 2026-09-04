# Effective C++：条款 13～17 学习笔记

> 对应《Effective C++》（第三版）第三章 **Resource Management（资源管理）**。
>
> 本章的核心不是“如何写 `new` 和 `delete`”，而是更根本的问题：**资源由谁拥有？何时释放？异常、提前返回和多线程出现时，释放是否仍然可靠？** 现代 C++ 的默认答案是 RAII：让对象的生命周期自动管理资源，而不是让人记忆一条条清理路径。
>
> 本文保留原书的思维框架，并补充 `std::unique_ptr`、`std::shared_ptr`、`std::make_unique`、`std::make_shared`、C++17 求值顺序变化与操作系统资源的基本关系。代码示例可手敲，配套程序在 `code/item_13to17`。

---

## 先建立共同模型：什么是资源，什么是所有权

资源（resource）是“数量有限、获取后必须归还或释放”的东西。内存只是最常见的一种。

| 资源 | 常见获取动作 | 对应释放动作 | 忘记释放的后果 |
| --- | --- | --- | --- |
| 堆内存 | `new`、`malloc` | `delete`、`free` | 内存泄漏，长期运行会耗尽内存 |
| 文件描述符 | `open`、`socket`、`accept` | `close` | 文件/连接数耗尽，无法再打开连接 |
| 互斥锁 | `lock` | `unlock` | 死锁，其他线程永久等待 |
| 数据库事务 | begin transaction | commit / rollback | 数据不一致、锁长期占用 |
| 线程 | 创建 `std::thread` | `join` / `detach` | 程序终止或后台任务失控 |

### 三个角色：拥有者、借用者、转移者

理解资源管理时，不要只看指针长什么样，先问它扮演什么角色：

```text
拥有者（owner）     ：负责最终释放资源
借用者（borrower）  ：临时使用资源，不负责释放
转移者（transfer）  ：把“最终释放责任”从一个拥有者交给另一个
```

例如：

```cpp
std::unique_ptr<Widget> owner = std::make_unique<Widget>();
Widget* borrowed = owner.get(); // borrowed 只能观察/使用，绝不能 delete
```

`owner` 销毁时会删除 `Widget`；`borrowed` 只是非拥有的裸指针。裸指针本身**不携带所有权语义**，所以阅读 API 时尤其要小心。

### RAII：把“获取”和“释放”绑到对象生命周期

RAII 是 Resource Acquisition Is Initialization，常译为“资源获取即初始化”。它的实际含义是：

```text
构造函数成功完成 → 对象已经拿到资源，且对象不变量成立
析构函数执行       → 对象释放资源，且析构不抛异常
```

局部对象无论因正常 `return`、异常栈展开、`break`、`continue` 还是提前离开作用域，都会自动析构。因此只要资源放进 RAII 对象，就不必为每一个出口手写清理。

```cpp
void writeReport() {
    std::lock_guard<std::mutex> lock(globalMutex); // 构造时 lock
    updateSharedState();
    if (shouldStop()) return; // 离开作用域时仍会自动 unlock
} // lock 的析构函数 unlock
```

这不是语法糖：它让编译器依据作用域插入析构调用，减少“某个异常路径漏掉 `unlock()`”的可能。

---

## 条款 13：以对象管理资源

### 核心结论

资源一旦获得，应立刻交给一个 RAII 对象管理。不要让资源长时间只存在于裸指针、裸句柄或“稍后再释放”的注释中。

### 为什么手写释放路径不可靠

```cpp
void oldStyle() {
    Widget* widget = new Widget;
    use(widget);
    delete widget;
}
```

上例表面正确，但以下任一情况都会漏掉 `delete`：

- `use(widget)` 抛异常；
- 中间新增了 `return`；
- 以后维护者新增一个分支，却忘记清理；
- 多个资源的获取与释放顺序变复杂。

RAII 改写后：

```cpp
void modernStyle() {
    auto widget = std::make_unique<Widget>();
    use(widget.get()); // 只在调用期间借用；所有权仍属于 widget
} // 正常返回或异常栈展开时，unique_ptr 自动 delete Widget
```

`std::unique_ptr<T>` 的析构函数会对所拥有的对象执行正确的删除操作；它不可复制，避免两个对象误以为自己都拥有同一资源。若函数需要接管所有权，调用方必须显式 `std::move(widget)`，这让“责任转移”在代码中可见。

### RAII 的因果链

```text
资源获取成功
  → 立即由局部 RAII 对象持有
  → 控制流离开作用域（正常或异常）
  → 编译器调用 RAII 对象析构函数
  → 资源被释放
```

关键前提是：资源管理对象的构造函数要么成功建立完整不变量，要么抛异常且不留下半管理资源；析构函数要尽力释放且不让异常逃出。这分别连接条款 4（初始化）、条款 8（析构异常）和本章。

### 操作系统层面的直觉

操作系统不会因为 C++ 某个指针变量离开作用域就自动关闭文件描述符或 socket。内核只知道某个进程持有一个整数句柄/对象引用；只有程序调用 `close`，或进程整体结束时内核回收进程资源，资源才会释放。

RAII 包装类的价值在于：把 `close(fd)` 放进析构函数，使 C++ 的作用域规则自动触发正确的系统调用。对长时间运行的服务，不能依赖“进程退出时内核最终会回收”，因为运行期间泄漏的 fd/socket 会先耗尽配额。

### `unique_ptr`、`shared_ptr` 与 `weak_ptr` 的基本选择

| 类型 | 所有权模型 | 何时销毁 | 默认选择场景 |
| --- | --- | --- | --- |
| `std::unique_ptr<T>` | 唯一拥有 | 唯一拥有者销毁/重置时 | 默认首选；资源有一个明确负责人 |
| `std::shared_ptr<T>` | 共享拥有（引用计数） | 最后一个 shared owner 消失时 | 多个对象确实需要共同延长生命周期 |
| `std::weak_ptr<T>` | 不拥有，只观察 shared 对象 | 不影响销毁 | 打破 `shared_ptr` 循环、可选观察 |

`shared_ptr` 不是“更安全的裸指针”。它解决的是共享生命周期，不自动解决谁能修改对象、对象本身是否线程安全、是否存在循环引用。两个对象互相持有 `shared_ptr` 会使引用计数永远不为零，应至少一侧改为 `weak_ptr`。

### 实用检查

1. 这份资源的唯一负责人是谁？若说不清，先不要写代码。
2. 能否直接用标准库 RAII 类型？例如容器、文件流、`std::lock_guard`、智能指针。
3. 若要自定义包装类，析构函数是否 non-throwing，移动后对象是否仍可安全析构？
4. 是否把借用的原始指针保存到了拥有者寿命之外？这会形成悬空指针。

---

## 条款 14：谨慎考虑资源管理类的复制行为

### 核心结论

资源管理对象被复制时，“资源责任”必须有明确策略。编译器默认逐成员复制并不知道你想共享、深拷贝、禁止复制，还是转移所有权。

### 复制一个拥有者，到底想表达什么？

假设类内部保存一个文件句柄或堆指针。`ResourceHandle b = a;` 至少可能有四种完全不同的业务含义：

| 策略 | 复制后关系 | 现代实现方向 | 常见场景 |
| --- | --- | --- | --- |
| 禁止复制 | 只有一个拥有者 | `unique_ptr`、`= delete` | mutex、独占文件、线程 |
| 共享同一资源 | 两个对象共同负责，最后一个释放 | `shared_ptr` | 共享不可变配置、共享缓存对象 |
| 深拷贝 | 得到两份独立资源 | 值成员、手写 copy | 图像、数据缓冲区、值对象 |
| 转移所有权 | 源对象交出资源 | 移动构造/移动赋值 | `unique_ptr`、可移动 RAII 句柄 |

原书写作时常见的 `std::auto_ptr` 会在复制时转移所有权；它的语义违反“复制后源对象仍可用”的直觉，已在 C++17 移除。现代 C++ 用 `std::unique_ptr` 明确禁止复制，并要求 `std::move` 显式转移。

### 三种安全示意

```cpp
// 1. 唯一拥有：复制编译失败，移动显式发生。
std::unique_ptr<Widget> one = std::make_unique<Widget>();
// auto two = one;             // 错误
auto two = std::move(one);     // two 接管；one 仍有效但通常为空

// 2. 共享拥有：复制 shared_ptr，引用计数加一。
auto sharedA = std::make_shared<Widget>();
auto sharedB = sharedA;

// 3. 深拷贝：vector 的复制会复制所有元素。
std::vector<int> valuesA{1, 2};
auto valuesB = valuesA;
valuesB[0] = 9; // valuesA[0] 仍为 1
```

### `shared_ptr` 引用计数背后发生什么

典型实现会让所有 `shared_ptr` 指向一个控制块（control block），其中保存强引用计数、弱引用计数、删除器等信息。复制 shared_ptr 时强计数递增；销毁时递减；强计数降到零时销毁受管对象。

计数的增减通常需要原子操作，以便不同线程持有的**不同 `shared_ptr` 对象**能安全地共同管理同一控制块。但这不表示受管 `T` 的普通成员可被多线程同时读写，也不表示对**同一个 shared_ptr 变量**的并发读写天然安全；对象访问和变量访问仍需要自己的同步。

引用计数还有性能和内存成本，所以“看不清谁拥有”时不能本能地换成 `shared_ptr`。先明确是否真的存在共享所有权；许多场景是“一个 owner + 多个短期 borrower”。

### 复制与赋值的差别在资源类中更明显

```cpp
ResourceHandle b = a; // b 从不存在到存在：拷贝构造
b = a;                // b 已持有旧资源：拷贝赋值
```

赋值时必须处理 b 原来的资源。对 shared ownership 是先正确调整引用计数；对深拷贝是先复制再替换以处理异常；对 unique ownership 则直接禁止复制。这正是条款 10～12 在资源管理类中的实际应用。

---

## 条款 15：在资源管理类中提供对原始资源的访问

### 核心结论

RAII 不意味着永远不能接触原始资源。很多旧 API、操作系统 API、C 库仍需要 `T*`、`FILE*`、fd 或原生句柄。资源管理类应提供受控访问方式，但调用者必须清楚：拿到的是**借用**，不是新的所有权。

### 为什么需要“逃生口”

```cpp
// 旧 C API，要求原始指针，但不接管所有权。
void legacyDraw(NativeImage* image);

class Image {
public:
    NativeImage* get() const noexcept { return image_.get(); }
private:
    std::unique_ptr<NativeImage> image_;
};

Image image;
legacyDraw(image.get()); // legacyDraw 临时借用；Image 仍负责 delete
```

`get()` 返回的裸指针只在拥有 `Image`/`unique_ptr` 仍存在且未 `reset`、未移动时有效。调用者绝不能：

```cpp
// delete image.get(); // 错误：会让 Image 之后再次 delete 同一资源
```

### 常见访问形式与语义

| 形式 | 例子 | 是否转移所有权 | 使用后谁负责释放 |
| --- | --- | --- | --- |
| 原始指针借用 | `unique.get()`、`shared.get()` | 否 | 原智能指针 |
| 解引用访问 | `*ptr`、`ptr->member()` | 否 | 原智能指针 |
| 原生句柄 | `file.native_handle()`、`socket.fd()` | 通常否 | RAII 包装类 |
| `release()` | `unique.release()` | **是** | 调用者必须立刻接管 |

最危险的是把 `get()` 与 `release()` 混淆：

```cpp
auto owner = std::make_unique<Widget>();
Widget* borrowed = owner.get(); // owner 仍负责释放

Widget* transferred = owner.release(); // owner 放弃责任；现在必须由新 owner 管理
std::unique_ptr<Widget> newOwner(transferred);
```

`release()` 很少需要；它常意味着 API 设计存在所有权边界。若必须使用，要在同一小段代码内立即把结果交给新的 RAII 对象，避免短暂裸拥有者导致异常泄漏。

### 用类型表达借用边界

现代接口中，若函数不拥有也不保存对象，可接受 `T&`、`const T&`、`T*`（可空借用）或 `std::span<T>`（连续数据借用）。若函数要保存或异步使用，就不能只接收短生命周期的借用；应接收拥有者、复制数据，或把生命周期规则写清楚。

这和条款 3 的 const 直接相关：`const T*` 表示“不能通过该指针修改”，却**不表示**“函数拥有它”或“它会一直存活”。const、所有权、生命周期是三个独立维度。

---

## 条款 16：成对使用 `new` 和 `delete` 时要采用相同形式

### 核心结论

`new T` 必须配 `delete`；`new T[n]` 必须配 `delete[]`。不匹配是未定义行为，即使元素类型简单、当前机器上似乎没出错，也不能依赖。

```cpp
Widget* one = new Widget;
delete one;

Widget* many = new Widget[10];
delete[] many;
```

### 为什么数组必须有另一种 delete

`new Widget[10]` 不只是分配“10 倍内存”；它还要构造 10 个 `Widget`。`delete[]` 必须知道这是数组，才能依次调用 10 次析构函数，再释放整块内存。

实现通常会在分配区域附近保存数组元素数量（常被称为 array cookie），供 `delete[]` 查找；这是常见 ABI 实现细节，不是 C++ 标准承诺的固定内存布局。无论实现怎样，程序员都必须让 delete 形式与 new 形式匹配。

```cpp
Widget* many = new Widget[10];
// delete many;   // 未定义行为：可能只析构一个元素、错误释放内存或崩溃
delete[] many;
```

### 容易忽略的 typedef/using 数组

```cpp
using AddressLines = std::string[4];

std::string* address = new AddressLines;
delete[] address; // 虽然源码没有写 []，AddressLines 本身是数组类型
```

关键是分配的**真实类型**，不是肉眼是否看见 `[]`。这类代码可读性差，实际项目更建议用 `std::array<std::string, 4>` 或 `std::vector<std::string>`，它们自己处理元素析构和内存释放。

### 智能指针也要匹配数组语义

```cpp
auto one = std::make_unique<Widget>();
auto many = std::make_unique<Widget[]>(10); // C++14 起支持
```

`std::unique_ptr<Widget>` 默认使用 `delete`；`std::unique_ptr<Widget[]>` 使用 `delete[]` 并提供数组下标访问。两者是不同类型，正是把“删除形式”交给类型系统。

不要把 `new/delete` 与 `malloc/free` 混用：前者会构造/析构对象并可能调用重载分配函数，后者只提供原始字节。应始终匹配 `new/delete`、`new[]/delete[]`、`malloc/free`。

### Placement new 是另一类问题

placement new 在已有内存上构造对象：

```cpp
alignas(Widget) std::byte storage[sizeof(Widget)];
Widget* widget = new (storage) Widget;
widget->~Widget(); // 显式结束对象生命周期；不能对 storage 使用普通 delete
```

它用于内存池、容器实现等低层场景，不是日常资源管理工具。初学阶段优先使用标准容器和智能指针。

---

## 条款 17：以独立语句将 `new` 得到的对象置入智能指针

### 核心结论

原书警告不要在一次函数调用的参数中同时写 `new` 和其他可能抛异常的表达式。安全做法是先让智能指针完整接管对象，再做其他操作。现代 C++ 更进一步：优先 `std::make_unique` / `std::make_shared`，直接避免裸 `new`。

### 原书为什么担心这一行

```cpp
processWidget(std::shared_ptr<Widget>(new Widget), priority());
```

在 C++11/14 的规则下，两个函数实参的求值顺序未指定。某个实现可能发生近似过程：

```text
1. 执行 new Widget，得到裸指针
2. 在 shared_ptr 来得及接管前，执行 priority()
3. priority() 抛异常
4. shared_ptr 没有构造完成，没有人 delete 那个 Widget → 泄漏
```

把步骤拆开后：

```cpp
auto widget = std::shared_ptr<Widget>(new Widget); // 此语句结束时，所有权已建立
processWidget(widget, priority());
```

若 `priority()` 抛异常，局部 `widget` 在栈展开时析构，Widget 会被删除。

### C++17 后的变化与仍然推荐的写法

C++17 起，函数实参的求值不再允许那种交错执行：一个实参的求值会完整结束后，才开始另一个实参。因此上面的“`new` 完成、但 shared_ptr 尚未构造就被另一个实参打断”的经典泄漏窗口已被语言规则消除。

但这不表示应重新手写 `shared_ptr<T>(new T)`：

```cpp
processWidget(std::make_shared<Widget>(), priority());
```

或更清楚地：

```cpp
auto widget = std::make_shared<Widget>();
processWidget(widget, priority());
```

仍然更好，因为它没有裸拥有指针，意图清晰，并通常能让对象和 shared_ptr 控制块一次分配。对唯一所有权用：

```cpp
auto widget = std::make_unique<Widget>();
```

### `make_shared` 不是无条件唯一选择

`make_shared` 通常一次分配对象和控制块，减少分配次数、提高缓存局部性；但如果存在 `weak_ptr` 长期观察者，对象析构后那一大块联合分配的内存可能要等弱计数也归零才完全释放。自定义删除器、需要指定特殊分配方式、构造函数访问受限等场景也可能需要直接构造 shared_ptr。

无论选择哪种，原则不变：**裸资源一产生，就应在同一完整表达式或更早的独立语句中交给 RAII 对象；不要让可能抛异常的其他工作插在中间。**

---

## 条款 13～17 的因果链

```text
资源有限且必须释放
        ↓
条款 13：用 RAII 对象把释放绑定到作用域
        ↓
条款 14：复制 RAII 对象时，必须定义所有权策略
        ↓
条款 15：向旧 API 暴露原始资源时，只能借用或明确转移
        ↓
条款 16：若仍直接管理内存，分配与释放形式必须匹配
        ↓
条款 17：资源产生后立刻由智能指针接管，避免异常窗口
```

## 写资源相关代码前的检查清单

1. 资源是什么？谁拥有最终释放责任？
2. 可否直接用 `std::vector`、文件流、`std::lock_guard`、`unique_ptr` 等现成 RAII 类型？
3. 是否真的需要 `shared_ptr`，还是一个 owner 加若干 borrower 就够？是否会形成循环引用？
4. API 返回的是拥有者还是借用者？`get()`、`release()` 的语义是否被明确区分？
5. 是否仍出现裸 `new`？能否替换为 `make_unique`、`make_shared` 或容器？
6. 若使用数组分配，删除形式是否为 `delete[]`？更好的容器选择是什么？
7. 任意步骤抛异常或提前 return 时，资源会自动释放吗？

---

## 配套可执行示例

示例在 [`code/item_13to17`](/home/jason/study/ck_study/cplusplus/effective_c++/code/item_13to17) 中。每个文件可单独编译：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic item13_raii.cpp -o item13
./item13
```

| 条款 | 文件 | 观察重点 |
| --- | --- | --- |
| 13 | `item13_raii.cpp` | 异常离开作用域时，RAII 对象仍自动释放资源。 |
| 14 | `item14_copying_resource.cpp` | 唯一拥有、共享拥有、深拷贝三种复制语义。 |
| 15 | `item15_raw_resource_access.cpp` | `get()` 借用原始指针与 `release()` 转移所有权。 |
| 16 | `item16_matching_new_delete.cpp` | `new/delete`、`new[]/delete[]` 与 `unique_ptr<T[]>`。 |
| 17 | `item17_exception_safe_creation.cpp` | 先由智能指针接管，再执行可能抛异常的操作。 |

建议先运行，再在调试器中观察构造/析构输出与局部变量的生命周期。不要取消示例中标为“错误”的注释并期待稳定结果；其中有些会触发未定义行为。

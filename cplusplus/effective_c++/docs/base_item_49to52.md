# Effective C++：条款 49～52 学习笔记

> 对应《Effective C++》（第三版）第八章 **Customizing `new` and `delete`（定制 new 和 delete）**。
>
> 这一章容易混淆，是因为源码中的 `new T`、名为 `operator new` 的函数，以及构造函数并不是同一件事。应先建立对象创建/销毁的完整流程，再学习失败处理、替换分配器和 placement new。
>
> 原书写于 C++11 之前。本文在原书原则上补充智能指针、对齐分配、sized delete、`std::allocator_traits`、`std::pmr`、`std::construct_at`、操作系统虚拟内存以及现代工程实践。配套示例默认使用 C++20。

---

## 0. 最重要的前置知识：`new` 表达式不等于 `operator new`

看到下面的代码：

```cpp
Widget* pointer = new Widget(42);
```

可以先用下列伪代码理解：

```cpp
void* rawMemory = operator new(sizeof(Widget)); // 只申请原始内存

try {
    Widget* pointer = 在 rawMemory 上执行 Widget(42); // 构造对象
    return pointer;
} catch (...) {
    operator delete(rawMemory);                 // 构造失败，归还内存
    throw;
}
```

真实语言规则比伪代码更严谨，但这个模型能解释本章的大多数问题：

- **new expression（new 表达式）**：`new Widget(42)` 这整段语法；它选择分配函数、取得内存、调用构造函数，并在构造失败时寻找对应的释放函数。
- **allocation function（分配函数）**：`operator new` 或 `operator new[]`；只取得一块满足大小和对齐要求的原始存储，不负责调用 `Widget` 构造函数。
- **constructor（构造函数）**：在已经取得的存储上开始对象生命周期并建立类的不变量。
- **delete expression（delete 表达式）**：`delete pointer`；通常先调用析构函数，再调用合适的 `operator delete` 释放存储。
- **deallocation function（释放函数）**：`operator delete` 或 `operator delete[]`；只归还存储，不等于析构函数。

销毁过程可简化为：

```text
delete pointer
    │
    ├─ pointer 非空：调用动态对象的析构函数
    │
    └─ 查找并调用合适的 operator delete，归还原始存储
```

因此，直接调用 `::operator new(100)` 得到的只是 100 字节原始存储，其中尚不存在 Widget 对象；直接调用 `::operator delete(memory)` 也不会替你执行 Widget 的析构函数。

### `new`、`malloc` 和智能指针的关系

`malloc` 只返回字节存储，不调用构造函数；`free` 不调用析构函数。普通 C++ 对象不要随意混用以下家族：

```text
new T       ↔ delete pointer
new T[n]    ↔ delete[] pointer
malloc      ↔ free
```

`new` 得到的对象在现代业务代码中通常应立即交给 RAII 所有者：

```cpp
auto widget = std::make_unique<Widget>(42);
auto shared = std::make_shared<Widget>(42);
```

这不会让本章知识失效。智能指针最终仍需销毁对象和释放内存；容器、分配器、对象池和底层库也都建立在同一对象模型上。只是日常代码不再裸露所有权。

## 缩写与术语速查

| 名称 | 含义 |
| --- | --- |
| RAII | Resource Acquisition Is Initialization，用对象生命周期管理资源 |
| OOM | Out Of Memory，内存不足 |
| ABI | Application Binary Interface，应用二进制接口，包含调用和对象布局等约定 |
| alignment | 对齐；对象起始地址需要满足的倍数要求 |
| allocator | 分配器；为容器提供存储分配模型 |
| PMR | Polymorphic Memory Resource，C++17 运行期多态内存资源框架 |
| arena/region | 区域分配：从较大内存区批量分配，通常统一释放 |
| pool | 内存池：复用某些尺寸的内存块 |
| placement new | 带额外实参的 `operator new`；狭义上常指在指定地址构造对象 |
| sized delete | 带 `std::size_t` 参数的释放函数，编译器可把对象大小传给它 |
| over-aligned | 对齐要求大于默认 new 保证值的类型 |

---

## 条款 49：了解 new-handler 的行为

### 1. 普通抛异常形式的 `operator new` 如何报告失败

```cpp
Widget* widget = new Widget;
```

若分配函数最终无法取得内存，通常抛出 `std::bad_alloc`。在抛出以前，它会查看当前的 **new-handler**。new-handler 是一个无参数、无返回值的函数，其类型为：

```cpp
using new_handler = void (*)();
```

可以安装处理函数：

```cpp
void outOfMemory();
std::new_handler old = std::set_new_handler(outOfMemory);
```

简化流程如下：

```text
operator new 尝试分配
        │
        ├─ 成功 → 返回地址
        │
        └─ 失败 → 当前 new-handler 是否存在？
                    │
                    ├─ 不存在 → 抛 std::bad_alloc
                    │
                    └─ 存在 → 调用 handler
                                  │
                                  └─ handler 返回后，再次尝试分配
```

最容易漏掉的是：**handler 正常返回，并不表示分配已经成功**。分配函数会再试；若条件没有改变，就会再次调用 handler，可能形成死循环。

### 2. 一个合格的 handler 能做什么

它必须让后续流程有明确变化，典型选择有：

1. 释放程序预留的 emergency buffer（紧急备用内存），然后返回，让分配重试；
2. 缩减缓存、通知内存池回收空闲页，然后返回；
3. 用 `std::set_new_handler` 安装另一个 handler；
4. 用 `std::set_new_handler(nullptr)` 卸载自己，使下一次失败抛 `std::bad_alloc`；
5. 自己抛出 `std::bad_alloc` 或它的派生异常；
6. 若程序不能继续，调用 `std::terminate`、`std::abort` 等终止。

handler 已经运行在内存紧张的路径中，应避免再进行可能分配内存的工作。例如构造复杂 `std::string`、向可能扩容的容器写日志，都可能递归触发分配失败。生产系统常预先准备固定缓冲区，并使用尽量不分配的错误输出路径。

### 3. `std::nothrow` 并非保证整个 new 表达式不抛异常

```cpp
Widget* widget = new (std::nothrow) Widget(arguments);
```

分配失败时，它通常返回空指针而非抛 `std::bad_alloc`：

```cpp
if (widget == nullptr) {
    // 处理分配失败
}
```

但 `std::nothrow` 只针对内存分配阶段。若 `Widget` 的构造函数因其他原因抛异常，该异常仍可传播。因此不要把它误解为“整句绝不会抛”。现代普通代码更常让 `bad_alloc` 传播到能够统一处理的边界，而不是每次 new 后手写空指针分支。

### 4. 类专属 handler 的原书思路

语言只提供当前全局 handler，没有原生的“每个类一个 handler”接口。原书的思路是：

```text
Widget::operator new
  → 暂时把全局 handler 换成 Widget 的 handler
  → 调用 ::operator new
  → 无论成功或抛异常，都恢复旧 handler（RAII）
```

配套 `item49_new_handler.cpp` 演示这一结构。恢复动作必须由 RAII guard 完成，否则 `::operator new` 抛异常时，旧 handler 永远得不到恢复。

现代多线程程序要额外谨慎：`set_new_handler` 本身有线程安全保证，不代表“临时修改全局策略”具有线程局部语义。在临时替换期间，其他线程的失败分配也可能看到这个 handler。因此，大型系统更倾向显式内存资源、组件级 allocator 或受控的 OOM 总策略，而不是频繁切换进程全局 handler。

### 5. 操作系统层面：`new` 成功不等于物理内存已全部到位

常见用户态分配器会管理自己的堆区，并可能通过 `brk`、`mmap` 或平台等价机制向 OS 请求虚拟地址空间。这是常见实现，不是 C++ 标准规定。

在采用虚拟内存、按需提交或 overcommit 的系统中：

```text
operator new 返回虚拟地址
        ↓
程序稍后首次写某个页面
        ↓
发生缺页，由 OS 提供物理页或执行内存策略
```

所以某些环境下，大额 `new` 可能先成功，真正访问页面时才暴露系统内存压力，甚至由 OS 终止进程。new-handler 只能处理分配函数明确报告的失败，不能保证拦截所有系统级 OOM 结果。

---

## 条款 50：了解替换 new 和 delete 的合理时机

### 1. “替换”有不同作用域

```cpp
void* operator new(std::size_t size);          // 全局替换版本

class Widget {
public:
    static void* operator new(std::size_t size); // 类专属版本
};
```

- 全局替换会影响使用可替换全局分配函数的广泛代码，风险和验证范围很大；程序中只能有一套相应定义，并涉及链接和 ABI。
- 类专属版本只在为该类（以及名字查找可能涉及的派生类）执行 new 时参与，更容易限定影响范围。
- 容器 allocator 或 `std::pmr::memory_resource` 把策略作为依赖显式传入，通常比改写全局机制更容易组合和测试。

### 2. 合理动机一：检测使用错误

自定义分配层可以记录：

- 分配大小、地址、线程、时间和调用位置；
- 是否出现 double free（重复释放）；
- 是否释放了不属于该分配器的地址；
- 块前后 guard bytes 是否被越界写坏；
- 程序结束时哪些分配仍未释放。

但现代工程应先考虑 AddressSanitizer、LeakSanitizer、Valgrind、平台堆诊断器和 profiler。它们通常比临时手写全局 `new/delete` 更完整，也较少改变被测程序行为。自定义追踪适合有专用格式、嵌入式环境或引擎基础设施等明确需求。

### 3. 合理动机二：改善特定分配模式的性能

通用分配器需要处理各种大小、线程和生命周期。若业务有强约束，可以使用：

- 固定大小对象池：例如大量同尺寸消息节点；
- arena/monotonic allocator：一次申请大块区域，小对象只移动指针，最终整区释放；
- 分线程缓存：减少全局锁竞争；
- 批量分配和释放：减少进入通用分配器的次数；
- NUMA/设备内存策略：让数据靠近使用它的处理器或设备。

代价也必须计算：内部碎片、峰值常驻内存、跨线程释放、对象生命周期约束、对齐、异常安全和调试难度。必须用真实 workload 基准测试，而不是看到 `operator new` 就默认自制内存池更快。

### 4. 合理动机三：统计、布局和安全策略

还可能为了：

- 按模块统计内存预算；
- 让相关对象更连续，提高 cache locality；
- 使用共享内存、持久化内存或特定设备内存；
- 在释放时清零敏感数据；
- 按特殊边界对齐 SIMD 或 cache line 数据。

注意“释放前清零”可能被优化器删除，需要平台安全清零原语；自定义分配器也不能自动解决 use-after-free 等全部安全问题。

### 5. 现代 C++ 更常见的显式方案：allocator 与 PMR

标准容器以 allocator 参数描述存储来源：

```cpp
std::vector<int, MyAllocator<int>> values;
```

allocator 是编译期类型参数，不同 allocator 类型会形成不同容器类型。实现自定义 allocator 时应通过 `std::allocator_traits` 使用，不要只照抄旧式接口。

C++17 PMR 把内存资源变成运行期对象：

```cpp
#include <memory_resource>

std::byte buffer[4096];
std::pmr::monotonic_buffer_resource arena{buffer, sizeof buffer};
std::pmr::vector<int> values{&arena};
```

> `PMR`: Polymorphic Memory Resource 运行时可替换的内存资源

示例：

```cpp
std::array<std::byte, 1024> localBuffer{}; // std::byte这是一块原始内存字节，不是拿来做数字运算的字符

栈： localBuffer
低地址
↓
┌───────────────────────────────┐
│                               │
│          1024 bytes           │
│                               │
└───────────────────────────────┘
                                ↑
                              高地址

std::pmr::monotonic_buffer_resource arena{
    localBuffer.data(),
    localBuffer.size(),
    std::pmr::null_memory_resource()
};

std::pmr // polymorphic memory resource
monotonic_buffer_resource // 只往前分配、不逐块回收的 arena allocator

要16B时：
┌────16────┬─────────────────────────────┐
│   used   │ free                        │
└──────────┴─────────────────────────────┘
           ↑
        current

给你当前位置 + 指针向后移动      
```

```cpp
std::pmr::null_memory_resource()
返回一个特殊的 std::pmr::null_memory_resource() // 永远拒绝内存分配。
一个永远不给你内存的备用资源

如果不写这个默认是： 从upstream_resource 中再申请更多的内存

localBuffer
   ↓
空间够？
 ├─ 是 → 从 localBuffer 分
 │
 └─ 否
      ↓
 upstream_resource
      ↓
 再申请新的内存块

------

当前流程：
arena.allocate()

      ↓

localBuffer 还有空间吗？

 ├─ 有
 │   ↓
 │  直接使用 localBuffer
 │
 └─ 没有
     ↓
 null_memory_resource
     ↓
 “不给”
     ↓
 throw std::bad_alloc

std::pmr::vector<int> values{&arena};
当前pmr只影响values不会污染全局
```

因果关系是：

```text
容器需要内存
  → polymorphic_allocator 转给 memory_resource
  → resource 决定从栈上缓冲区、池或上游资源取得内存
  → 容器接口基本不变，策略可以在运行期注入
```

`monotonic_buffer_resource` 的单次 deallocate 通常不回收具体小块，而是在资源重置/销毁时批量回收，适合阶段性生命周期；不适合要求每次 erase 都立刻归还内存的长期容器。

配套 `item50_custom_allocation.cpp` 用类专属函数演示计数与作用域限制，同时用 PMR 展示现代的显式资源注入。教学计数器不是生产分配器。

---

## 条款 51：编写 new 和 delete 时遵守惯例

一旦亲自实现分配函数，你就在实现底层基础设施，必须满足调用者和语言规则的预期，而不只是“malloc 一下”。

### 1. `operator new` 的基本责任

抛异常形式的典型逻辑可以抽象为：

```cpp
void* operator new(std::size_t size) {
    if (size == 0) size = 1;

    while (true) {
        if (void* memory = try_allocate(size)) {
            return memory;
        }

        std::new_handler handler = std::get_new_handler();
        if (!handler) throw std::bad_alloc{};
        handler();
    }
}
```

这只是说明控制流，不能直接当作完整全局替换实现。真正实现还需满足标准规定的大小、对齐、线程安全和替换函数要求，并避免自身递归分配。

为什么零字节也要处理？调用 `operator new(0)` 时，成功结果仍应是可供随后正确释放的非空指针；实现常把 0 提升为 1。注意 `new T[0]` 还涉及数组表达式和可能的实现记账，不能用“数组有零个元素”简单推断底层请求一定为 0。

### 2. 返回内存必须满足对齐要求

地址若不满足 `alignof(T)`，在上面构造 T 就会产生未定义行为。普通全局 `operator new(size)` 覆盖默认支持范围；C++17 对 over-aligned 类型引入带 `std::align_val_t` 的形式：

```cpp
void* operator new(std::size_t size, std::align_val_t alignment);
void operator delete(void* memory, std::align_val_t alignment) noexcept;
```

若类是 `alignas(64)`，类专属分配函数不应把对齐参数丢掉。配套示例验证返回地址能被 64 整除。

### 3. 类专属分配器要考虑派生类

类中的 allocation function 是 static，并且名称会被派生类查找到：

```cpp
class Base {
public:
    static void* operator new(std::size_t size);
};

class Derived : public Base {
    int extra[100];
};

Derived* object = new Derived; // 可能进入 Base::operator new(sizeof(Derived))
```

如果 Base 的池只保存 `sizeof(Base)` 大小的块，却忽略传入的 size，就会给 Derived 一块过小内存，随后构造越界写入。常见防线是：

```cpp
if (size != sizeof(Base)) {
    return ::operator new(size);
}
```

真正固定块池只处理恰好等于 Base 大小的请求，其他大小委托给全局分配器。更简单的设计是把不应继承的池化类型标为 `final`。

### 4. `operator delete` 的责任

释放函数通常应：

- 是 `noexcept`，绝不能让异常逃离；
- 只接收由匹配分配机制产生、尚未释放的地址；
- 对可能出现的空指针路径保持安全；
- 使用与申请时相匹配的后端和对齐信息；
- 不读取已经结束生命周期的对象字段，除非元数据明确位于仍有效的分配块中。

数组形式必须成对：`operator new[]` 对应 `operator delete[]`。数组分配可能包含实现使用的 array cookie，用于记录析构元素数量；不要假设 `new T[n]` 得到的用户指针前面一定恰好是什么布局。

### 5. unsized、sized 与 aligned delete

你可能见到这些形式：

```cpp
void operator delete(void* pointer) noexcept;
void operator delete(void* pointer, std::size_t size) noexcept;
void operator delete(void* pointer, std::align_val_t alignment) noexcept;
void operator delete(void* pointer, std::size_t size,
                     std::align_val_t alignment) noexcept;
```

带 size 的版本让分配器可能更快定位尺寸类别；带 alignment 的版本用于对齐分配。编译器实际选择哪个释放函数受可见声明、对象类型、构造是否失败以及语言规则影响。不要只实现一半后假设所有编译器总会调用你期望的形式。

### 6. 不要在分配器内部无意递归

下面的全局替换思路可能递归：

```cpp
void* operator new(std::size_t size) {
    std::cout << "allocate " << size; // iostream 内部也可能分配
    // 将记录放入 std::vector        // vector 扩容再次调用 operator new
    // ...
}
```

底层记录通常需要预分配缓冲、原子计数、平台原语或明确的递归保护。多线程下还要处理同步和进程退出阶段的静态对象生命周期。

---

## 条款 52：写了 placement new，也要写对应的 placement delete

### 1. placement new 有广义和狭义两种说法

只要 `operator new` 除了第一个 `std::size_t` 外还有参数，它就是广义的 placement allocation function：

```cpp
static void* operator new(std::size_t size, std::ostream& log);
```

日常口语中的“placement new”通常特指标准库提供的指定地址形式：

```cpp
void* buffer = /* 已经取得且满足大小、对齐的存储 */;
Widget* widget = new (buffer) Widget(42);
```

这个标准 placement new 不申请新内存，只返回传入地址，然后 new 表达式在该地址构造 Widget。

### 2. 为什么需要匹配的 placement delete

考虑带日志参数的分配函数：

```cpp
class Widget {
public:
    static void* operator new(std::size_t size, std::ostream& log);
};

Widget* widget = new (std::cerr) Widget;
```

流程是：

```text
operator new(size, std::cerr) 成功取得内存
                         ↓
                  Widget 构造函数
                    ├─ 成功 → 返回 Widget*
                    └─ 抛异常 → 对象没有构造完成，谁归还刚才的内存？
```

编译器会寻找参数能够匹配的 placement delete：

```cpp
static void operator delete(void* memory, std::ostream& log) noexcept;
```

这里的额外参数列表必须与 placement new 对应。若找不到合适函数，构造异常仍会向外传播，但刚取得的内存无法经这条自动清理路径归还，于是可能泄漏。

### 3. placement delete 只负责“构造失败”路径

假设构造成功：

```cpp
Widget* widget = new (std::cerr) Widget;
delete widget;
```

普通 `delete widget` 不会保存最初的 `std::cerr` 实参，因此不会调用 `operator delete(void*, std::ostream&)`；它使用普通 delete 查找规则。类通常仍需提供普通 `operator delete(void*)`。

这就是条款 52 容易误解的地方：匹配 placement delete 主要是 new 表达式已经分配成功、但构造函数随后抛异常时的自动回滚函数。

### 4. 声明一个 class-specific new 会隐藏其他形式

如果类只声明：

```cpp
class Widget {
public:
    static void* operator new(std::size_t, std::ostream&);
};
```

那么普通 `new Widget` 可能因类作用域名称查找而不可用，并不会自动退回所有全局重载。工程中常用一个基类或在类中成组提供所需形式：

```text
普通 new/delete
nothrow new/delete（若需要）
自定义 placement new/delete
对齐形式（若类型需要）
```

不要随意设计第二参数恰好为 `std::size_t` 的 placement delete，因为它可能与语言已有的 sized ordinary delete 形式发生语义冲突。

### 5. 在已有缓冲区中构造对象的现代写法

C++20 可使用 `std::construct_at` 和 `std::destroy_at` 明确表达生命周期：

```cpp
alignas(Widget) std::byte storage[sizeof(Widget)];

Widget* widget = std::construct_at(
    reinterpret_cast<Widget*>(storage), 42);

std::destroy_at(widget);
```

这里 storage 本身是自动存储期字节数组，所以不要再 `delete widget`：

```text
storage 提供存储       → 离开作用域自动消失
construct_at 开始对象生命周期
destroy_at 结束对象生命周期
```

若存储来自某个 allocator，现代泛型代码通常通过 `std::allocator_traits<Allocator>::construct/destroy`（实现会使用相应构造机制）并最终由同一 allocator deallocate。存储来源、对象生命周期和归还方式必须分别配对。

配套 `item52_placement_delete.cpp` 同时演示构造抛异常时匹配 placement delete 自动运行，以及 `construct_at/destroy_at` 对外部缓冲区的管理。

---

## 四个条款的完整逻辑链

```text
new T(args)
  │
  ├─ 选择 operator new，申请满足 size/alignment 的原始存储
  │      ├─ 失败：new-handler 重试、抛 bad_alloc 或终止（49）
  │      ├─ 为诊断/性能/特殊内存而定制（50）
  │      └─ 必须遵守大小、对齐、派生类、异常等惯例（51）
  │
  └─ 在存储上构造 T
         ├─ 成功：对象生命周期开始
         │          └─ 将来析构 + ordinary delete
         └─ 抛异常：调用与本次 operator new 匹配的 operator delete
                    └─ 自定义 placement new 必须有匹配 delete（52）
```

## 现代工程中的选择顺序

遇到内存分配需求时，可以按下面顺序判断：

1. 普通独占对象：优先 `std::make_unique`；共享所有权确实存在时才用 `std::make_shared`。
2. 连续同类元素：优先标准容器，不要手写 `new[]`。
3. 需要诊断内存错误：先用 sanitizer、Valgrind 或平台 profiler。
4. 某组容器需要阶段性内存资源：考虑 `std::pmr`。
5. 类型确有固定尺寸、高频分配热点：测量后考虑类专属池或成熟分配库。
6. 只有整个进程都需要统一策略，且能验证所有 ABI、对齐、线程和工具链交互时，才考虑替换全局 `new/delete`。

“能重载”不等于“应该重载”。对大多数业务类，保持默认分配并用 RAII 管理所有权更安全。

## 学习检查清单

1. 能否解释 `new Widget`、`operator new` 和 `Widget::Widget` 的区别？
2. new-handler 返回后，分配器为什么可能再次调用它？
3. `new (std::nothrow) T` 的构造函数还能不能抛异常？
4. 自定义分配策略是全局、类专属、allocator 还是 PMR 更合适？
5. 固定块分配器收到 `sizeof(Derived)` 时会不会错误地只给 `sizeof(Base)`？
6. over-aligned 类型是否保留了 alignment 信息？
7. 分配与释放是否使用同一家族、同一资源和对应数组形式？
8. placement new 取得内存后构造失败，哪个 placement delete 会被寻找？
9. 外部缓冲区中的对象何时 `destroy_at`，底层存储又由谁释放？

## 配套可执行示例

代码位于 [`code/item_49to52`](../code/item_49to52/)。四个文件都有自己的 `main`，应分别编译，不要一起链接。

```bash
cd /home/jason/study/ck_study/cplusplus/effective_c++/code/item_49to52

g++ -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
    item49_new_handler.cpp -o /tmp/item49
/tmp/item49
```

| 条款 | 文件 | 重点观察 |
| --- | --- | --- |
| 49 | `item49_new_handler.cpp` | 类专属 handler、RAII 恢复旧 handler、模拟失败后的重试 |
| 50 | `item50_custom_allocation.cpp` | 类专属计数作用域，以及 PMR 从栈上 arena 分配 |
| 51 | `item51_new_delete_conventions.cpp` | 派生类大小转交全局分配器、64 字节对齐分配 |
| 52 | `item52_placement_delete.cpp` | 构造异常自动调用匹配 placement delete；`construct_at/destroy_at` |

进一步核对语言规则可阅读标准草案中的 [new 表达式](https://eel.is/c++draft/expr.new)、[delete 表达式](https://eel.is/c++draft/expr.delete)、[存储分配与释放](https://eel.is/c++draft/basic.stc.dynamic) 和 [new/delete 库支持](https://eel.is/c++draft/new.delete)。

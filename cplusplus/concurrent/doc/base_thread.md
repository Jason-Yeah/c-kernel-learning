# C++ 线程基础 (Thread Basics)

基于《C++ 并发编程实战》（*C++ Concurrency in Action*）整理。

---

## 前置概念：进程、线程与主/子关系

### 进程（Process）

**进程**是操作系统资源分配的基本单位，拥有独立的地址空间（代码段、数据段、堆）、文件描述符表、信号处理表等。进程间默认不共享内存，需要通过 IPC（管道、共享内存、消息队列等）通信。

**主进程**指的是程序启动时由操作系统创建的初始进程（如 `./a.out` 运行时产生的进程 PID）。在 C++ 中，`main()` 函数就运行在主进程的主线程中。

### 线程（Thread）

**线程**是操作系统调度的基本单位，隶属于某个进程。同一进程内的线程共享进程的资源（地址空间、文件描述符等），但各有独立栈和寄存器上下文。

| 对比维度 | 进程 | 线程 |
|---------|------|------|
| 资源拥有 | 独立地址空间（4GB 虚拟空间） | 共享进程地址空间 |
| 创建开销 | 大（fork 复制页表、文件描述符等） | 小（仅分配栈和 TCB） |
| 通信方式 | IPC（管道/共享内存/信号等） | 直接读写共享变量 |
| 隔离性 | 高（一个进程崩溃不影响其他） | 低（野指针可垮整个进程） |
| 上下文切换 | 慢（涉及地址空间切换、TLB 刷新） | 快（同一地址空间，TLB 命中率高） |

### 主线程 vs 子线程

**主线程（Main Thread）**：
- C++ 程序启动时，`main()` 函数所在的线程
- 程序入口点，负责初始化、启动子线程、等待子线程完成
- 如果主线程退出，整个进程结束——所有子线程会被操作系统强行终止（无论是否执行完毕）

**子线程/工作者线程（Child/Worker Thread）**：
- 由 `std::thread` 或底层 API（`pthread_create`、`CreateThread`）创建的线程
- 在主线程之外并行执行任务
- 生命周期受限于主进程——主进程结束，所有子线程随进程一起消亡

```
时间轴 ─────────────────────────────────────────►
                                                   
进程启动                                            
  │                                                 
  ├─ 主线程 (main) ─────┬──┬──┬──┬──┬────── 等待 join ──── 进程退出
  │                     │  │  │  │  │                     
  ├─ 子线程 1           └──┴─────────── 执行完毕 ── join 成功
  │                                         
  ├─ 子线程 2                  └──── 执行完毕 ── join 成功
```

### 主进程 vs 子进程（Linux `fork()`）

**子进程**是通过 `fork()` 系统调用从当前进程复制出的新进程。与子线程的关键区别：

| 对比 | 子线程 (std::thread) | 子进程 (fork()) |
|------|---------------------|----------------|
| 创建方式 | `std::thread(func)` | `fork()` |
| 地址空间 | 共享 | 复制（写时拷贝 COW） |
| PID | 相同（同一进程） | 不同 |
| 创建开销 | ~微秒级（仅分配 TCB + 栈） | ~毫秒级（复制页表） |
| 同步方式 | mutex / atomic / 条件变量 | 共享内存 / 信号量 / 管道 |
| 独立崩溃 | 可能拖垮整个进程 | 独立存在 |

> **C++ 并发编程中我们主要关注线程而非进程**。进程间通信属于操作系统级话题，本书和本文档聚焦于 `std::thread` 和相关同步原语。

---

## 目录

- [1. 线程基本概念](#1-线程基本概念)
- [2. std::thread 原理与构造](#2-stdthread-原理与构造)
- [3. 线程生命周期与管控](#3-线程生命周期与管控)
- [4. 异常安全与线程](#4-异常安全与线程)
- [5. 互斥锁 std::mutex](#5-互斥锁-stdmutex)
- [6. 死锁](#6-死锁)
- [7. std::unique_lock](#7-stdunique_lock)
- [8. 读写锁 shared_mutex](#8-读写锁-shared_mutex)
- [9. 递归锁 recursive_mutex](#9-递归锁-recursive_mutex)
- [10. std::call_once](#10-stdcall_once)
- [11. 底层原理](#11-底层原理)
- [12. 附录：完整可运行示例](#12-附录完整可运行示例)

---

## 1. 线程基本概念

### 1.1 什么是线程

线程是操作系统能够进行运算调度的**最小单位**。一个进程可以包含多个线程，它们共享进程的地址空间（代码段、数据段、堆），但每个线程拥有独立的：

- **栈**（Stack）—— 局部变量、函数调用链
- **寄存器上下文** —— PC（程序计数器）、SP（栈指针）、通用寄存器
- **TLS**（Thread Local Storage）—— 线程局部存储
- **errno** —— 每个线程有自己的错误码

```
┌─────────────────────────────┐
│          进程 (Process)      │
│  ┌───────┐ ┌───────┐ ┌───┐ │
│  │ 代码段 │ │ 数据段 │ │ 堆 │ │  ← 共享
│  └───────┘ └───────┘ └───┘ │
│  ┌──────┐ ┌──────┐ ┌────┐  │
│  │栈(T1)│ │栈(T2)│ │... │  │  ← 独立
│  └──────┘ └──────┘ └────┘  │
└─────────────────────────────┘
```

### 1.2 C++ 线程抽象

`std::thread` 是 C++11 引入的跨平台线程抽象，它封装了底层操作系统的线程 API：

| 平台 | 底层 API |
|------|----------|
| Linux | `pthread_create` / `clone` 系统调用 |
| Windows | `CreateThread` |
| macOS | `pthread_create` |

`std::thread` 本质上是对 `pthread_t`（POSIX）或 `HANDLE`（Windows）的 RAII 封装。

---

## 2. std::thread 原理与构造

### 2.1 头文件

```cpp
#include <thread>
```

### 2.2 构造方式

```cpp
#include <iostream>
#include <thread>
#include <functional>

// 1. 默认构造 —— 不关联任何线程（not joinable）
std::thread t0;

// 2. 函数指针
void hello() { std::cout << "Hello\n"; }
std::thread t1(hello);

// 3. 可调用对象（函数对象 / 仿函数）
struct Work {
    void operator()() const { std::cout << "Work\n"; }
};
std::thread t2(Work{});

// ⚠️ 注意 Most Vexing Parse：
// std::thread t2(Work());   ← 被解析为函数声明！
// 用 {} 或额外括号避免：std::thread t2(Work{});
// C++11引入：{}列表不可被解析未函数声明
/**
* 原因：声明了一个名为 t2 的函数，该函数返回 std::thread，并接受一个参数：
* 该参数是一个指向函数的指针，这个被指向的函数返回 Work 且不接受任何参数
*/

// 4. Lambda 表达式
std::thread t3([] {
    std::cout << "Lambda\n";
});

// 5. 成员函数指针
class Task {
public:
    void run(int n) { std::cout << n << '\n'; }
};
Task obj;
std::thread t4(&Task::run, &obj, 42);

// 6. 带参数的可调用对象
void print_sum(int a, int b) {
    std::cout << a + b << '\n';
}
std::thread t5(print_sum, 3, 4);

t1.join(); t2.join(); t3.join(); t4.join(); t5.join();
```

### 2.3 参数传递的陷阱

```cpp
#include <thread>
#include <string>

void modify(std::string& s) { s += " modified"; }

void example() {
    // ❌ 编译错误：参数默认按值传递，不能将临时绑定到 non-const 引用
    // std::thread t(modify, "hello");

    // ✅ 正确：用 std::ref 显式传递引用
    std::string s = "hello";
    std::thread t(modify, std::ref(s));
    t.join();
    // s 现在是 "hello modified"
}
```

> **原理**：`std::thread` 的构造函数会将所有参数**按值 decay 拷贝**到内部存储中，再传递给可调用对象。如果需要传递引用，必须用 `std::ref` / `std::cref` 包裹。这是为了确保线程安全——如果传递了指向局部变量的指针/引用，而线程还没执行完局部变量就销毁了，会出现悬空引用。

### 2.4 内部原理示意图

```
std::thread t(func, arg)
      │
      ▼
  分配内部存储（decay_copy 参数）
      │
      ▼
  创建底层线程（OS 级别）
      │
      ├─ Linux:  pthread_create(&native_handle, ...)
      │           └─ 内部调用 clone() 系统调用
      │
      ├─ Windows: CreateThread(...)
      │
      ▼
  native_handle() 可获取底层句柄
```

---

## 3. 线程生命周期与管控

### 3.1 join() 与 detach()

| 方法 | 行为 | 后果 |
|------|------|------|
| `t.join()` | 阻塞**当前线程**，等待 `t` 执行完毕 | `t` 变为 not joinable |
| `t.detach()` | 将 `t` 与 `std::thread` 对象分离，允许它独立运行 | `t` 变为 not joinable，成为**守护线程** |
| `t.joinable()` | 检查 `t` 是否关联了一个可等待的线程 | `true` 表示可以 join/detach |

```cpp
#include <iostream>
#include <thread>
#include <chrono>

void worker(int id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Worker " << id << " done\n";
}

int main() {
    std::thread t1(worker, 1);

    // 检查是否可 join
    if (t1.joinable()) {
        t1.join();  // 等待 t1 完成
    }

    // detach 示例
    std::thread t2(worker, 2);
    t2.detach();          // t2 在后台继续运行
    // t2.join();         // ❌ 危险！detach 后不能再 join

    // 注意：detach 后线程可能仍在运行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return 0;
}
```

### 3.2 所有权转移（Move Semantics）

`std::thread` 是**只移型别**（move-only），不能拷贝：

```cpp
#include <thread>
#include <utility>

void f() {}

int main() {
    std::thread t1(f);

    // ❌ 不能拷贝
    // std::thread t2 = t1;

    // ✅ 可以移动
    std::thread t2 = std::move(t1);
    // 此时 t1 变为 not joinable，t2 管理线程

    // 函数间传递
    auto create_thread = []() -> std::thread {
        return std::thread(f);  // 移动构造返回值
    };

    std::thread t3 = create_thread();

    // 容器中管理多个线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.push_back(std::thread(f));
    }
    for (auto& t : threads) {
        t.join();
    }

    t2.join();
    t3.join();
}
```

### 3.3 RAII 封装 —— 线程守卫

始终用 RAII 包装 `std::thread`，避免忘记 join/detach 导致 `std::terminate`：

```cpp
#include <thread>
#include <utility>
#include <iostream>

class ThreadGuard {
    std::thread t_;

public:
    explicit ThreadGuard(std::thread t) : t_(std::move(t)) {
        if (!t_.joinable())
            throw std::logic_error("Non-joinable thread");
    }

    ~ThreadGuard() {
        if (t_.joinable())
            t_.join();  // 析构时自动 join
    }

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

void task() {
    std::cout << "Running in thread\n";
}

int main() {
    try {
        ThreadGuard guard(std::thread(task));
        // 即使这里抛出异常，guard 析构也会 join 线程
        throw std::runtime_error("oops");
    } catch (...) {
        // 线程已被正确 join
    }
    return 0;
}
```

> **为什么 std::thread 析构会调用 terminate？**
>
> C++ 标准规定：如果一个 `std::thread` 处于 `joinable` 状态（即关联了一个仍在运行的底层线程），但析构时既没有 `join()` 也没有 `detach()`，程序会调用 `std::terminate()` 终止。这是为了防止"线程泄漏"——一个无法再被管理的线程在后台永远运行。

### 3.4 线程生命周期状态机

```
        ┌──────────────────────────────────┐
        │  std::thread t   (默认构造)      │
        │  不关联底层线程，joinable()=false │
        └────────┬─────────────────────────┘
                 │ std::thread(func)
                 ▼
        ┌──────────────────────────────────┐
        │  关联底层线程，joinable()=true   │
        └──────┬───────────────┬───────────┘
               │               │
          t.join()         t.detach()
               │               │
               ▼               ▼
        ┌────────────┐  ┌────────────┐
        │ joinable() │  │ joinable() │
        │ = false    │  │ = false    │
        │ 等待完成   │  │ 后台运行   │
        └────────────┘  └────────────┘
               │               │
         析构(安全)       析构(安全)
```

---

## 4. 异常安全与线程

### 4.1 问题场景

```cpp
void problematic() {
    std::thread t(do_work);
    // 如果这里抛出异常，t 的析构会调用 terminate()！
    do_other_work();  // 可能抛异常
    t.join();         // 不会执行到这里
}
```

### 4.2 解决方案一：try/catch

```cpp
void try_catch_solution() {
    std::thread t(do_work);
    try {
        do_other_work();
    } catch (...) {
        t.join();  // 确保异常路径也 join
        throw;
    }
    t.join();
}
```

### 4.3 解决方案二：RAII（推荐）

前面写的 `ThreadGuard` 就是标准做法。C++ 核心指南也推荐使用 RAII 管理线程。

### 4.4 解决方案三：C++20 std::jthread

C++20 引入 `std::jthread`（joining thread），析构时自动 join：

```cpp
#include <thread>

void f() { /* ... */ }

void cpp20_solution() {
    std::jthread t(f);  // 析构时自动 join
    // 即使抛出异常也安全
    do_other_work();
}
// t 析构时自动调用 t.join()
```

`std::jthread` 还支持**协作式取消**（`request_stop`），比原始 `std::thread` 更安全易用。

---

## 5. 互斥锁 std::mutex

### 5.1 问题：数据竞争

```cpp
#include <iostream>
#include <thread>

int counter = 0;
constexpr int N = 100000;

void increment() {
    for (int i = 0; i < N; ++i)
        ++counter;  // ❌ 数据竞争！结果不确定
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join(); t2.join();
    std::cout << counter << " (期望 " << 2*N << ")\n";
    // 输出可能为 142837 之类的值，每次不同
}
```

> **底层原因**：`++counter` 在汇编层面是三条指令：
> ```asm
> mov   eax, [counter]   ; 从内存加载到寄存器
> add   eax, 1           ; 寄存器加 1
> mov   [counter], eax   ; 写回内存
> ```
> 两个线程同时执行这三条指令，会出现**丢失更新**：
> ```
> 时间 →  T1: load(0) → add(1) → store(1)
>         T2: load(0) → add(1) → store(1)
> 结果: counter = 1  (期望 2)
> ```

### 5.2 使用 std::mutex

```cpp
#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;
std::mutex mtx;            // 全局互斥锁
constexpr int N = 100000;

void safe_increment() {
    for (int i = 0; i < N; ++i) {
        mtx.lock();        // 加锁
        ++counter;         // 临界区
        mtx.unlock();      // 解锁
    }
}

int main() {
    std::thread t1(safe_increment);
    std::thread t2(safe_increment);
    t1.join(); t2.join();
    std::cout << counter << '\n';  // 正确输出 200000
}
```

### 5.3 std::lock_guard —— RAII 封装

```cpp
void safe_increment_v2() {
    for (int i = 0; i < N; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // 构造时 lock
        ++counter;
    }                                           // 析构时 unlock
}
```

> **原则**：始终用 `std::lock_guard` 或 `std::unique_lock` 管理 mutex，不要裸调 `lock()`/`unlock()`。忘记 unlock 会导致死锁或资源泄漏。

### 5.4 mutex 底层原理

```
std::mutex::lock()
    │
    ▼
 尝试获取 futex（快速用户态互斥锁）
    │
    ├─ 成功 → 直接进入临界区（无系统调用，约 10-30ns）
    │
    └─ 失败 → 进入内核态（约 1-10μs）
                │
                ▼
          futex(FUTEX_WAIT) 系统调用
                │
                ▼
          线程被挂起，上下文切换
          OS 调度其他线程运行
```

在 Linux 上，`std::mutex` 基于 **futex**（Fast Userspace Mutex）实现：

1. **用户态快速路径**：用原子操作尝试获取锁（`LOCK CMPXCHG` 指令），无竞争时**零系统调用**
2. **慢路径**：有竞争时通过 `futex` 系统调用让线程睡眠，避免忙等待

> **futex 核心思想**：大多数情况下没有锁竞争，所以大多数情况不需要进内核。只有真正有竞争时才走慢路径。

---

## 6. 死锁

### 6.1 什么是死锁

两个或多个线程互相等待对方持有的锁，导致所有线程永久阻塞：

```
线程 A: lock(m1) → lock(m2)  // 持有 m1，等待 m2
线程 B: lock(m2) → lock(m1)  // 持有 m2，等待 m1
```

### 6.2 死锁的四个必要条件

| 条件 | 说明 |
|------|------|
| 1. 互斥 | 资源一次只能被一个线程占用 |
| 2. 持有并等待 | 线程持有资源的同时等待其他资源 |
| 3. 不可剥夺 | 线程已获得的资源不能被强制剥夺 |
| 4. 循环等待 | 存在循环等待链 |

### 6.3 死锁示例

```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex m1, m2;

void func_a() {
    std::lock_guard<std::mutex> lk1(m1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lk2(m2);  // 可能死锁
    std::cout << "func_a\n";
}

void func_b() {
    std::lock_guard<std::mutex> lk2(m2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lk1(m1);  // 可能死锁
    std::cout << "func_b\n";
}
```

### 6.4 解决方案一：固定加锁顺序

始终按**相同顺序**加锁：

```cpp
void func_a() {
    std::lock_guard<std::mutex> lk1(m1);
    std::lock_guard<std::mutex> lk2(m2);
    // ...
}

void func_b() {
    std::lock_guard<std::mutex> lk1(m1);  // 跟 func_a 顺序一致！
    std::lock_guard<std::mutex> lk2(m2);
    // ...
}
```

### 6.5 解决方案二：std::lock() —— 原子加锁

`std::lock()` 可以一次性锁住多个 mutex，避免死锁：

```cpp
#include <mutex>

void safe_func() {
    std::lock(m1, m2);  // 同时锁住 m1 和 m2（或一个都不锁）

    // 用 adopt_lock 表示锁已被持有，lock_guard 不要再次 lock
    std::lock_guard<std::mutex> lk1(m1, std::adopt_lock);
    std::lock_guard<std::mutex> lk2(m2, std::adopt_lock);

    // 临界区...
}
```

`std::lock()` 内部使用 **try-lock 退避算法**：先锁第一个，尝试锁第二个，失败则释放第一个，稍后再试。这样避免了循环等待。

### 6.6 解决方案三：层次锁（自定义）

大型项目中可以给锁分配层次，强制加锁顺序：

```cpp
class HierarchicalMutex {
    const unsigned long level_;
    unsigned long previous_level_;
    static thread_local unsigned long this_thread_level_;

public:
    explicit HierarchicalMutex(unsigned long level) : level_(level), previous_level_(0) {}

    void lock() {
        if (this_thread_level_ <= level_) {
            throw std::logic_error("mutex hierarchy violated");
        }
        previous_level_ = this_thread_level_;
        this_thread_level_ = level_;
        internal_mutex_.lock();
    }

    void unlock() {
        this_thread_level_ = previous_level_;
        internal_mutex_.unlock();
    }

private:
    std::mutex internal_mutex_;
};

thread_local unsigned long HierarchicalMutex::this_thread_level_ = ULONG_MAX;
```

用法：层次值大的先锁（外层），小的后锁（内层），违反顺序就抛异常。

---

## 7. std::unique_lock

### 7.1 与 lock_guard 的区别

| 特性 | `std::lock_guard` | `std::unique_lock` |
|------|-------------------|-------------------|
| RAII 加锁/解锁 | ✅ | ✅ |
| 手动 lock/unlock | ❌ | ✅ |
| 延迟加锁 (defer) | ❌ | ✅ |
| try_lock | ❌ | ✅ |
| 超时加锁 | ❌ | ✅ |
| 转移所有权 | ❌ | ✅ |
| 性能开销 | 零额外开销 | 稍大（维护更多状态） |

### 7.2 延迟加锁（deferred locking）

```cpp
std::mutex mtx;
std::unique_lock<std::mutex> lock(mtx, std::defer_lock);  // 创建时不加锁
// ... 做一些准备工作 ...
lock.lock();     // 手动加锁
// 临界区
lock.unlock();   // 手动解锁（可选，析构也会自动解锁）
```

### 7.3 try_lock —— 非阻塞尝试

```cpp
std::mutex mtx;

void try_example() {
    std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
    if (lock.try_lock()) {
        // 成功获取锁
        // 临界区...
    } else {
        // 锁被占用，做其他事
    }
}
```

### 7.4 带超时的 try_lock（需要 std::timed_mutex）

```cpp
std::timed_mutex tmtx;

void timed_example() {
    std::unique_lock<std::timed_mutex> lock(tmtx, std::defer_lock);
    if (lock.try_lock_for(std::chrono::milliseconds(100))) {
        // 100ms 内成功获取锁
    } else {
        // 超时未获取到
    }

    // 或者到某个时间点
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    if (lock.try_lock_until(deadline)) {
        // 在 deadline 前成功获取锁
    }
}
```

### 7.5 所有权转移

```cpp
std::mutex mtx;

std::unique_lock<std::mutex> get_lock() {
    std::unique_lock<std::mutex> lock(mtx);
    // ... 准备数据 ...
    return lock;  // 转移锁的所有权
}

void process() {
    auto lock = get_lock();  // 获取锁的同时也获得了保护
    // 访问共享数据...
}  // lock 析构，自动解锁
```

这个模式在某些设计中很有用——调用者不关心锁的细节，只需要拿到一个"锁"对象就知道数据受保护。

---

## 8. 读写锁 shared_mutex

### 8.1 适用场景

**多个读线程并发安全，写线程独占**：

| 场景 | 并发度 |
|------|--------|
| 只有读操作 | 多个线程可以同时读 |
| 有写操作 | 所有读线程必须等待，写线程独占 |

### 8.2 shared_mutex（C++17）

```cpp
#include <shared_mutex>
#include <thread>
#include <vector>

class ThreadSafeCounter {
    mutable std::shared_mutex mtx_;
    int value_ = 0;

public:
    // 读操作 —— 共享锁
    int get() const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return value_;
    }

    // 写操作 —— 独占锁
    void increment() {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        ++value_;
    }

    void reset() {
        std::lock_guard<std::shared_mutex> lock(mtx_);
        value_ = 0;
    }
};
```

### 8.3 性能对比

```
读多写少场景下：
  std::mutex        ─── 读和读之间也互斥，性能差
  std::shared_mutex ─── 读读不互斥，性能好

写多读少场景下：
  shared_mutex 的管理开销可能比普通 mutex 还大
  → 此时直接用 std::mutex 更合适
```

### 8.4 升级锁（Upgrade Lock）的概念

C++ 标准库**没有**直接的升级锁（upgrade_lock），但可以通过组合实现：

```cpp
// 读 → 写 升级：先释放 shared_lock，再获取 unique_lock
// 注意：两个操作之间不是原子的！数据可能已变化
void upgrade_example(ThreadSafeCounter& c) {
    int val;
    {
        std::shared_lock lock(c.mtx_);
        val = c.value_;
    }  // shared_lock 解锁
    // ⚠️ 风险：另一个线程可能在这期间修改了 value_

    {
        std::unique_lock lock(c.mtx_);
        // 需要重新检查数据一致性
        if (c.value_ == val) {
            c.value_ += 10;
        }
    }
}
```

> Boost 库提供了 `boost::upgrade_lock` 和 `boost::upgrade_to_unique_lock` 支持真正的升级锁操作。

---

## 9. 递归锁 recursive_mutex

### 9.1 问题场景

同一个线程试图对一个**非递归 mutex** 重复加锁：

```cpp
std::mutex mtx;

void inner() {
    std::lock_guard<std::mutex> lk(mtx);  // ❌ 同一线程第二次加锁 → 死锁！
}

void outer() {
    std::lock_guard<std::mutex> lk(mtx);
    inner();  // inner 也尝试加锁 mtx
}
```

### 9.2 使用 recursive_mutex

```cpp
std::recursive_mutex rmtx;

void inner_recursive() {
    std::lock_guard<std::recursive_mutex> lk(rmtx);  // ✅ 同一个线程可以重复加锁
}

void outer_recursive() {
    std::lock_guard<std::recursive_mutex> lk(rmtx);
    inner_recursive();  // 正常
}
```

### 9.3 性能与设计考量

| | `std::mutex` | `std::recursive_mutex` |
|---|---|---|
| 同一线程重复加锁 | 死锁 | 成功（计数+1） |
| 解锁次数 | 1 次 | 与加锁次数相同 |
| 性能 | 快 | 稍慢（需维护计数器） |
| 适用场景 | 大多数情况 | 递归调用、已有接口设计缺陷 |

> **建议**：`recursive_mutex` 通常是不良设计的征兆。更好的解决方式是重构代码，**不要在持有锁时调用也可能加锁的函数**。如果不确定，先尝试重构，确认无法避免再用递归锁。

```cpp
// 更好的设计：提取不需要锁的部分
class Widget {
    std::mutex mtx_;
    int data_;

    // 私有的无锁实现
    void do_update_unlocked(int x) { data_ = x; }

public:
    void update(int x) {
        std::lock_guard<std::mutex> lk(mtx_);
        do_update_unlocked(x);
    }

    void batch_update(int a, int b) {
        std::lock_guard<std::mutex> lk(mtx_);
        do_update_unlocked(a);
        do_update_unlocked(b);  // 调用无锁版本，不需要递归锁
    }
};
```

---

## 10. std::call_once

### 10.1 用途

只执行一次的线程安全初始化，比如**单例模式**、**懒加载**：

```cpp
#include <mutex>

std::once_flag flag;

void init() {
    std::cout << "初始化一次\n";
}

void worker() {
    std::call_once(flag, init);  // 无论多少线程调用，init 只执行一次
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    t1.join(); t2.join(); t3.join();
    // 输出：初始化一次
}
```

### 10.2 与双重检查锁定对比

```cpp
// ❌ 错误的双重检查锁定（有 data race）
static Widget* instance = nullptr;
Widget* get_instance() {
    if (!instance) {           // 未加锁读取
        std::lock_guard lk(mtx);
        if (!instance) {
            instance = new Widget();
        }
    }
    return instance;
}
```

问题：编译器和 CPU 可能重排指令，导致另一个线程看到 `instance` 非空但对象未构造完成。

```cpp
// ✅ C++11 后正确的方式 —— Meyers Singleton
Widget& get_instance() {
    static Widget instance;  // C++11 保证静态局部变量初始化线程安全
    return instance;
}

// ✅ 或使用 call_once
static Widget* instance = nullptr;
std::once_flag inst_flag;
Widget* get_instance() {
    std::call_once(inst_flag, [] { instance = new Widget(); });
    return instance;
}
```

---

## 11. 底层原理

### 11.1 Linux 线程创建 —— clone 系统调用

在 Linux 上，`std::thread` 最终调用 `clone()` 系统调用（而不是 `fork()`）：

```c
// clone() 与 fork() 的区别：
// fork()   → 复制整个进程地址空间（写时复制）
// clone()  → 与调用者共享地址空间，创建"线程"
clone(CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD,
      child_stack, ...);
```

| 标志 | 含义 |
|------|------|
| `CLONE_VM` | 共享内存空间（父子进程用同一份地址空间） |
| `CLONE_FS` | 共享文件系统信息（umask、cwd） |
| `CLONE_FILES` | 共享文件描述符表 |
| `CLONE_SIGHAND` | 共享信号处理函数 |
| `CLONE_THREAD` | 加入同一线程组 |

这就是"线程"与"进程"的本质区别——**是否共享地址空间**。

### 11.2 上下文切换的成本

当 OS 从一个线程切换到另一个线程时：

```
线程 A 运行                  线程 B 运行
    │                           │
    ├─ 时间片用完               │
    ├─ 或系统调用阻塞            │
    │                           │
    ▼                           │
 陷阱入内核态                    │
    │                           │
 保存 A 的寄存器:                │
   PC, SP, RAX, RBX, ...        │
    │                           │
 切换 MMU 页表（同一进程不需）    │
    │                           │
 加载 B 的寄存器:                │
   PC, SP, RAX, RBX, ...        │
    │                           │
 返回用户态                      │
    │                           ▼
    │                        B 继续执行
```

**典型成本**（现代 CPU）：

| 操作 | 大约时间 |
|------|----------|
| 函数调用 | ~1-5 ns |
| 原子操作 (LOCK XADD) | ~10-30 ns |
| 系统调用 (syscall) | ~100-500 ns |
| 线程上下文切换 | ~1-10 μs (=1000-10000 ns) |
| 进程上下文切换（含 TLB 刷新） | ~10-100 μs |

> 一次上下文切换大约是 **10000 条指令** 的时间。这就是为什么"过多线程"反而会拖慢程序——线程切换的开销可能超过实际计算。

### 11.3 mutex 的汇编级实现

一个简单的自旋锁（spinlock）在 x86-64 上的实现：

```asm
; spinlock (x86-64)
lock:
    mov   eax, 1         ; 想要获取锁
    xchg [mutex], eax    ; 原子交换：将 [mutex] 置 1，同时读取旧值
    test  eax, eax       ; 旧值是 0 吗？
    jnz   lock           ; 不是 → 锁被占用，继续循环
    ret                  ; 是  → 成功获取锁
unlock:
    mov   [mutex], 0     ; 解锁
    ret
```

> `xchg` 指令在 x86 上**自动带 LOCK 前缀**（即使不写 `LOCK`），保证多核间原子性。现代 `std::mutex` 使用更高效的 `LOCK CMPXCHG`（Compare-and-Swap）和用户态 futex 优化。

### 11.4 内存序与可见性

```cpp
// 这是一个有问题的"标志位"模式
bool ready = false;
int data = 0;

// 线程 A
void producer() {
    data = 42;
    ready = true;     // 编译器可能优化：把 data=42 挪到 ready=true 之后！
}

// 线程 B
void consumer() {
    while (!ready);   // 可能一直看不到 ready 变为 true
    // 即使看到了 ready == true，data 可能还是 0！
}
```

**原因**：编译器和 CPU 会重排指令。C++ 内存模型需要显式的同步机制：

```cpp
#include <atomic>

std::atomic<bool> ready{false};
int data = 0;

// 线程 A
void producer() {
    data = 42;
    ready.store(true, std::memory_order_release);  // 释放语义：前面的写入对其他线程可见
}

// 线程 B
void consumer() {
    while (!ready.load(std::memory_order_acquire)); // 获取语义：看到 ready 后，data 也可见
    // data 一定是 42
}
```

> **acquire-release 语义**：线程 A 的 `release` 写入与线程 B 的 `acquire` 读取形成**同步关系**（happens-before），保证 A 在 release 之前写入的所有数据在 B acquire 之后都可见。

### 11.5 伪共享（False Sharing）

```cpp
// ❌ 伪共享：两个变量在同一个缓存行
struct alignas(64) Bad {
    int a;           // 线程 A 频繁写
    int b;           // 线程 B 频繁写
    // 64 字节对齐确保不跨缓存行
};
```

**缓存行（Cache Line）**：x86-64 通常 64 字节。CPU 以缓存行为单位加载/同步数据。

```
核心 1 (写 a)          核心 2 (写 b)
    │                       │
    ├─ 加载缓存行           ├─ 加载缓存行
    │  (包含 a 和 b)        │  (包含 a 和 b)
    │                       │
    ├─ 写 a                 ├─ 写 b
    │                       │
    ├─ 缓存行标记 Modified  ├─ 但我的缓存行已过期！
    │                       │
    │    ←── 缓存一致性协议 (MESI) ──→
    │       缓存行在核心间来回失效
    │       导致性能骤降（比各自独立慢 10x+）
```

**解决方法**：用 `alignas(64)` 或填充（padding）让不同线程访问的变量落在不同缓存行。

```cpp
// ✅ 用 padding 避免伪共享
struct alignas(64) ThreadData {
    int value;
    char padding[60];  // 填充到 64 字节
};

ThreadData data_a;  // 在一个缓存行
ThreadData data_b;  // 在另一个缓存行
```

### 11.6 Thread Local Storage (TLS)

```cpp
// 每个线程独立的一份拷贝
thread_local int tls_counter = 0;

void worker() {
    tls_counter++;  // 只修改当前线程的副本
    std::cout << tls_counter;  // 每个线程都输出 1
}
```

**底层原理**：TLS 通过 **TLS 寄存器**（x86-64 上是 `FS` 段寄存器）实现：

```asm
; 访问 thread_local 变量
mov   rax, fs:[0]       ; 获取 TLS 基址（每个线程不同）
mov   ecx, [rax + offset]  ; 偏移量由链接器确定
```

每个线程在栈附近分配一块 TLS 区域，`FS` 寄存器指向这块区域的基址。同一个变量名在不同线程中实际上指向不同的物理地址。

---

## 12. 附录：完整可运行示例

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <vector>
#include <cassert>

// ============== ThreadGuard ==============
class ThreadGuard {
    std::thread t_;
public:
    explicit ThreadGuard(std::thread t) : t_(std::move(t)) {}
    ~ThreadGuard() { if (t_.joinable()) t_.join(); }
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// ============== 线程安全的计数器 ==============
class ThreadSafeCounter {
    mutable std::shared_mutex mtx_;
    int count_ = 0;
public:
    int get() const {
        std::shared_lock lock(mtx_);
        return count_;
    }
    void increment() {
        std::unique_lock lock(mtx_);
        ++count_;
    }
};

// ============== 测试 ==============
int main() {
    constexpr int N = 100000;
    ThreadSafeCounter counter;

    // 创建多个线程并发增加计数器
    auto worker = [&] {
        for (int i = 0; i < N; ++i)
            counter.increment();
    };

    std::vector<ThreadGuard> guards;
    guards.reserve(4);
    for (int i = 0; i < 4; ++i)
        guards.emplace_back(std::thread(worker));

    // ThreadGuard 析构时自动 join
    // 这里手动触发（离开作用域）
    // 实际项目中用 RAII 即可
    // （也可以显式让 guards 析构）

    // 等待所有 ThreadGuard 析构
    // 由于 guards 在 main 返回前析构，我们不需要额外操作
    // 但为了演示，手动清空
    {
        std::vector<ThreadGuard> tmp;
        tmp.swap(guards);
    }  // tmp 析构，自动 join 所有线程

    std::cout << "最终计数: " << counter.get()
              << " (期望: " << 4 * N << ")\n";
    assert(counter.get() == 4 * N);
    return 0;
}
```

编译运行：

```bash
g++ -std=c++20 -Wall -Wextra -pthread demo.cpp -o demo && ./demo
```

> **注意**：Linux 下编译多线程程序必须加 `-pthread` 链接 pthread 库。Windows 下 `g++` 也支持，MSVC 默认已支持。

---

## 13. 标准库源码精读

> 以下源码来自 GCC 13 libstdc++，路径 `/usr/include/c++/13/`。
> 通过阅读标准库的实现，理解设计理念和底层原理。

### 13.1 std::thread 源码分析

#### 核心类定义 (`bits/std_thread.h`)

```cpp
// /usr/include/c++/13/bits/std_thread.h
class thread {
public:
    using native_handle_type = __gthread_t;  // ≡ pthread_t (Linux)

    class id {
        native_handle_type _M_thread;        // 存储原生线程 ID
    public:
        id() noexcept : _M_thread() { }      // 默认构造 → 空 ID
        explicit id(native_handle_type __id) : _M_thread(__id) { }
    };

private:
    id _M_id;  // 线程的唯一标识
```

**设计理念**：
- `id` 是 `thread` 的核心成员，默认构造的 `_M_thread` 为零值，表示**未关联线程**
- `joinable()` 的实现就是检查 `_M_id == id()`（即 ID 是否为空）
- 空 ID = 默认构造 / 已被 move 走 / 已 join / 已 detach

#### 构造函数——线程创建的关键

```cpp
// /usr/include/c++/13/bits/std_thread.h
template<typename _Callable, typename... _Args,
         typename = _Require<__not_same<_Callable>>>
explicit
thread(_Callable&& __f, _Args&&... __args)
{
    // 1. 编译期检查：参数必须可调用
    static_assert( __is_invocable<...>::value,
      "std::thread arguments must be invocable after conversion to rvalues" );

    // 2. 创建 Call_wrapper：将所有参数 decay 拷贝后打包成 tuple
    using _Wrapper = _Call_wrapper<_Callable, _Args...>;

    // 3. 创建状态对象 + 启动线程
    _M_start_thread(_State_ptr(new _State_impl<_Wrapper>(
          std::forward<_Callable>(__f), std::forward<_Args>(__args)...)),
        _M_thread_deps_never_run);
}
```

**关键设计——Call_wrapper 内部机制**：

```cpp
// _Call_wrapper 实际上是 _Invoker<tuple<decay_t<_Callable>, decay_t<_Args>...>>
template<typename... _Tp>
    using _Call_wrapper = _Invoker<tuple<typename decay<_Tp>::type...>>;

template<typename _Tuple>
    struct _Invoker {
        _Tuple _M_t;  // 存储 decay 拷贝后的函数对象 + 参数

        // 通过 index_sequence 展开 tuple，依次调用
        template<size_t... _Ind>
        typename __result<_Tuple>::type
        _M_invoke(_Index_tuple<_Ind...>)
        { return std::__invoke(std::get<_Ind>(std::move(_M_t))...); }

        typename __result<_Tuple>::type
        operator()() {
            using _Indices = _Build_index_tuple<tuple_size<_Tuple>::value>;
            return _M_invoke(_Indices());
        }
    };
```

**为什么参数要 decay 拷贝？**  
因为线程可能比创建它的函数活得久，局部变量的引用可能在线程使用前就销毁了。所以标准强制 `thread` 构造函数对所有参数做 `decay` 拷贝（实际上等同于按值传递），存到 `tuple` 中。

**参数传递陷阱验证**：
```cpp
void f(int& x) { x = 42; }
int a;
std::thread t(f, a);         // ❌ 编译错误：f(int&) 但传入的是 int（decay 后为 int）
std::thread t(f, std::ref(a));  // ✅ std::ref 绕过 decay 拷贝
```

#### _M_start_thread —— 底层 pthread_create 调用

```cpp
// 对应 libstdc++ 编译后的库代码（src/c++11/thread.cc）
// 核心逻辑（伪码还原）：
void thread::_M_start_thread(_State_ptr state, void (*)()) {
    // 1. 分配原生线程属性
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // 2. 创建 pthread，传递的 func 是静态的 _State::_M_run 包装器
    pthread_t native_id;
    int ret = pthread_create(&native_id, &attr,
                             &_dispatch_thread,  // 静态函数
                             state.get());       // 用户态参数

    // 3. 保存原生线程 ID
    if (ret == 0)
        _M_id = id(native_id);
    else
        throw system_error(ret, system_category());
}
```

**`_dispatch_thread` 的职责**（静态函数）：
```c
// 相当于：
void* _dispatch_thread(void* p) {
    auto* state = static_cast<thread::_State*>(p);
    state->_M_run();  // 实际调用用户传入的 callable
    delete state;
    return nullptr;
}
```

**设计模式**：**Type Erasure（类型擦除）**  
`_State` 是抽象基类，`_State_impl<_Callable>` 是模板派生类。`pthread_create` 只需要 C 函数指针 `void* (*)(void*)`，通过虚函数 `_M_run()` 恢复类型信息：

```
pthread_create           pthread 线程入口
    │                        │
    ▼                        ▼
_dispatch_thread(void* p) → state->_M_run() → 用户的 callable
                                    ▲
                      _State_impl<Lambda> 重写 _M_run()
```

这就是为什么 `std::thread` 可以接受任意可调用对象——**虚函数 + 静态分发**。

#### join() 的实现

```cpp
// src/c++11/thread.cc（伪码）
void thread::join() {
    if (!joinable())
        throw system_error(EDEADLK);  // 不能 join 一个非 joinable 的线程

    // 检查是否自己 join 自己
    if (get_id() == this_thread::get_id())
        throw system_error(EDEADLK);

    pthread_join(_M_id._M_thread, nullptr);  // 阻塞等待子线程结束
    _M_id = id();  // 置空，此后 joinable() 返回 false
}
```

**关键行为**：
- `pthread_join` 阻塞当前线程，直到目标线程退出
- join 后 **必须** 将 `_M_id` 置空，否则析构时会 `std::terminate`
- 自己 join 自己会导致死锁，标准库直接抛 `EDEADLK`

#### detach() 的实现

```cpp
// src/c++11/thread.cc（伪码）
void thread::detach() {
    if (!joinable())
        throw system_error(EINVAL);

    pthread_detach(_M_id._M_thread);  // 标记为 detached 状态
    _M_id = id();  // 置空，释放所有权
}
```

`pthread_detach` 告诉内核：该线程结束时自动回收资源（不需要其他线程 join）。detach 后线程在后台继续运行，`std::thread` 对象放弃所有权。

#### 析构函数——为什么 joinable 会 terminate

```cpp
~thread() {
    if (joinable())
        std::__terminate();  // 直接终止进程！
}
```

**设计理由**：  
如果析构时自动 `join`，可能导致隐式阻塞——你不知在哪里就卡住了。如果析构时自动 `detach`，线程可能在后台运行而局部变量已被销毁，导致数据竞争。标准选择了最安全（也最严厉）的做法：**让程序员显式处理线程生命周期**。

---

### 13.2 std::mutex 源码分析

#### 类型映射

```cpp
// /usr/include/x86_64-linux-gnu/c++/13/bits/gthr-posix.h
typedef pthread_t __gthread_t;              // thread::native_handle_type
typedef struct timespec __gthread_time_t;   // 超时时间结构体
#define __GTHREAD_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER  // 静态初始化
```

#### mutex 类定义

```cpp
// /usr/include/c++/13/bits/std_mutex.h
class __mutex_base {
protected:
    typedef __gthread_mutex_t __native_type;  // ≡ pthread_mutex_t

#ifdef __GTHREAD_MUTEX_INIT
    __native_type _M_mutex = __GTHREAD_MUTEX_INIT;  // 静态初始化
    constexpr __mutex_base() noexcept = default;
#else
    __native_type _M_mutex;
    __mutex_base() noexcept {
        __GTHREAD_MUTEX_INIT_FUNCTION(&_M_mutex);  // 动态初始化
    }
    ~__mutex_base() noexcept { __gthread_mutex_destroy(&_M_mutex); }
#endif
};
```

**设计要点**：
- Linux 上通过 `PTHREAD_MUTEX_INITIALIZER` **静态初始化**，零开销
- 没有定义析构函数（静态初始化不需要清理）
- 没有动态分配内存——`pthread_mutex_t` 直接内嵌在对象中

#### lock() / unlock() 的实现

```cpp
class mutex : private __mutex_base {
public:
    void lock() {
        int __e = __gthread_mutex_lock(&_M_mutex);  // → pthread_mutex_lock
        if (__e)
            __throw_system_error(__e);  // 出错时抛出 system_error
    }

    bool try_lock() noexcept {
        return !__gthread_mutex_trylock(&_M_mutex);  // → pthread_mutex_trylock
        // 返回 true 表示锁成功
    }

    void unlock() {
        __gthread_mutex_unlock(&_M_mutex);  // → pthread_mutex_unlock
    }
};
```

**gthread 宏展开**（Linux x86-64）：
```c
// gthr-posix.h 中通过 __gthrw 宏做 weak symbol 声明
__gthrw(pthread_mutex_lock)     // → extern __typeof(pthread_mutex_lock) pthread_mutex_lock __attribute__((weak));
#define __gthread_mutex_lock(x) __gthrw_(pthread_mutex_lock)(x)
```

**为什么要用 weak symbol？**  
支持单线程环境：如果 pthread 库未链接，这些符号解析为 `nullptr`，调用前通过 `__gthread_active_p()` 检查跳过。

#### pthread_mutex_lock 底层 (glibc/NPTL)

`pthread_mutex_lock` 在内核层面使用 **futex**（Fast Userspace Mutex）：

```c
// glibc nptl/pthread_mutex_lock.c（简化逻辑）
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    // 1. 原子尝试：CAS 将 0 改为 1（未锁→锁定）
    unsigned int v = atomic_exchange(&mutex->__data.__lock, 1);
    if (v == 0)
        return 0;  // 快速路径：锁成功！

    // 2. 慢速路径：需要阻塞
    if (v == 1 || /* 不等待 */) {
        do {
            // futex 系统调用：如果锁仍被占用则睡眠
            futex(&mutex->__data.__lock, FUTEX_WAIT, 2, ...);
            v = atomic_exchange(&mutex->__data.__lock, 2);
        } while (v != 0);
    }
    return 0;
}
```

**futex 的精髓**：
- **无竞争时**：仅一次原子操作（~10ns），无系统调用
- **有竞争时**：通过 `futex(FUTEX_WAIT)` 陷入内核，线程挂起（~μs 级别）
- 值 0=未锁，1=锁定无竞争者，2=锁定有等待者

---

### 13.3 lock_guard 源码分析

```cpp
template<typename _Mutex>
    class lock_guard {
    public:
        typedef _Mutex mutex_type;

        // 构造时加锁
        explicit lock_guard(mutex_type& __m) : _M_device(__m)
        { _M_device.lock(); }

        // adopt_lock：假设调用方已持有锁，不再加锁
        lock_guard(mutex_type& __m, adopt_lock_t) noexcept : _M_device(__m)
        { }

        // 析构时解锁（永不抛异常——unlock() 不抛）
        ~lock_guard()
        { _M_device.unlock(); }

        // 禁止拷贝
        lock_guard(const lock_guard&) = delete;
        lock_guard& operator=(const lock_guard&) = delete;

    private:
        mutex_type&  _M_device;  // 引用绑定（不能为空，不可移动）
    };
```

**设计理念——最小且安全的 RAII 包装器**：
- 只有构造和析构两个操作，没有 `lock()`/`unlock()` 接口
- 发生异常时栈展开会调用析构函数，保证锁一定释放
- `_M_device` 用**引用**而非指针——永远不可能是 `nullptr`，避免空指针检查
- 不可移动、不可拷贝——生命周期严格绑定到栈上

**对比 unique_lock**：`lock_guard` 没有 `defer_lock` / `try_lock` / 所有权转移等功能，但编译器可以内联得更好，因为它只有构造+析构两个出口。

---

### 13.4 unique_lock 源码分析

```cpp
template<typename _Mutex>
    class unique_lock {
    public:
        typedef _Mutex mutex_type;

    private:
        mutex_type* _M_device;  // 指针（可为空，表示未关联 mutex）
        bool        _M_owns;    // 是否持有锁
```

**与 lock_guard 的关键差异**：
| 特性 | lock_guard | unique_lock |
|------|-----------|-------------|
| 成员 | 引用 `mutex_type&` | 指针 `mutex_type*` |
| 可空 | 否 | 是（默认构造/被 move 后） |
| 可移动 | 否 | 是 |
| 额外操作 | 无 | `lock()`/`unlock()`/`try_lock()`/`release()` |

#### 各个构造器

```cpp
explicit unique_lock(mutex_type& __m)
    : _M_device(std::__addressof(__m)), _M_owns(false)
{
    lock();       // 调用的是 unique_lock::lock()
    _M_owns = true;
}

unique_lock(mutex_type& __m, defer_lock_t) noexcept
    : _M_device(std::__addressof(__m)), _M_owns(false)
{ }  // 不锁！等待后续手动 lock()

unique_lock(mutex_type& __m, try_to_lock_t)
    : _M_device(std::__addressof(__m)),
      _M_owns(_M_device->try_lock())
{ }  // 尝试锁，结果记录在 _M_owns

unique_lock(mutex_type& __m, adopt_lock_t) noexcept
    : _M_device(std::__addressof(__m)), _M_owns(true)
{ }  // 假设已持有锁，仅注册

// 移动构造：转让所有权
unique_lock(unique_lock&& __u) noexcept
    : _M_device(__u._M_device), _M_owns(__u._M_owns)
{
    __u._M_device = 0;   // 源对象置空
    __u._M_owns = false;
}
```

**`defer_lock_t` / `try_to_lock_t` / `adopt_lock_t` 是空标签类型**：

```cpp
struct defer_lock_t   { explicit defer_lock_t() = default; };
struct try_to_lock_t  { explicit defer_lock_t() = default; };
struct adopt_lock_t   { explicit defer_lock_t() = default; };

inline constexpr defer_lock_t   defer_lock{ };
inline constexpr try_to_lock_t  try_to_lock{ };
inline constexpr adopt_lock_t   adopt_lock{ };
```

**设计模式——Tag Dispatch（标签派发）**：  
通过不同的空结构体标签，在编译期选择不同的构造行为，无需运行时判断。

#### lock() 的安全检查

```cpp
void lock() {
    if (!_M_device)
        __throw_system_error(errc::operation_not_permitted);  // 空指针
    else if (_M_owns)
        __throw_system_error(errc::resource_deadlock_would_occur);  // 重复锁
    else {
        _M_device->lock();
        _M_owns = true;
    }
}
```

**安全检查**：unique_lock 在调用锁操作前先做防御性检查，防止：
1. 对空的 unique_lock（默认构造或已被 move）调用 `lock()`
2. 重复加锁（未解锁又加锁——注意这不是死锁检测，只是防止 misuse）

---

### 13.5 scoped_lock 源码分析 (C++17)

```cpp
// /usr/include/c++/13/mutex
template<typename... _MutexTypes>
    class scoped_lock {
    public:
        explicit scoped_lock(_MutexTypes&... __m) : _M_devices(std::tie(__m...))
        { std::lock(__m...); }     // 原子方式锁多个 mutex

        ~scoped_lock()
        { std::apply([](auto&... __m) { (__m.unlock(), ...); }, _M_devices); }
        // C++17 fold expression: 逐个解锁

    private:
        tuple<_MutexTypes&...> _M_devices;  // 存储所有 mutex 的引用
    };
```

**亮点**：
- 变参模板支持任意数量的 mutex
- 构造时调用 `std::lock()` 保证不产生死锁
- C++17 fold expression `(__m.unlock(), ...)` 逐个解锁
- `std::apply` 展开 tuple 调用 lambda

---

### 13.6 std::lock() 死锁避免算法

```cpp
// /usr/include/c++/13/mutex
template<typename _L1, typename _L2, typename... _L3>
    void lock(_L1& __l1, _L2& __l2, _L3&... __l3) {
#if __cplusplus >= 201703L
        if constexpr (is_same_v<_L1, _L2> && (is_same_v<_L1, _L3> && ...)) {
            // 所有锁类型相同 → 使用数组 + 轮询算法
            constexpr int _Np = 2 + sizeof...(_L3);
            unique_lock<_L1> __locks[] = {
                {__l1, defer_lock}, {__l2, defer_lock}, {__l3, defer_lock}...
            };
            int __first = 0;
            do {
                __locks[__first].lock();              // 锁当前锚点
                for (int __j = 1; __j < _Np; ++__j) {
                    const int __idx = (__first + __j) % _Np;
                    if (!__locks[__idx].try_lock()) {  // 尝试下一个
                        // 失败 → 解锁全部，换锚点重试
                        for (int __k = __j; __k != 0; --__k)
                            __locks[(__first + __k - 1) % _Np].unlock();
                        __first = __idx;
                        break;
                    }
                }
            } while (!__locks[__first].owns_lock());

            for (auto& __l : __locks) __l.release();
        } else
#endif
        {
            // 不同类型 → 递归旋转算法
            int __i = 0;
            __detail::__lock_impl(__i, 0, __l1, __l2, __l3...);
        }
    }
```

**算法核心**（旋转锁 + 回退）：
1. 从一个锁开始 `lock()`
2. 对剩余锁依次 `try_lock()`
3. 如果 `try_lock` 失败，**回退解锁所有已获锁**，将失败的那个锁作为新锚点
4. 循环直到所有锁都获得

**为什么能避免死锁**：死锁需要"每个线程持有一部分锁，等待另一部分"——本算法保证获得锁的操作是原子的（要么全部得到，要么全部释放再重试），不会被其他线程打断。

---

### 13.7 shared_mutex 源码分析

#### 实现选择

```cpp
// /usr/include/c++/13/shared_mutex
#if _GLIBCXX_USE_PTHREAD_RWLOCK_T
    // 优先使用 pthread_rwlock_t（Linux 上都有）
    class __shared_mutex_pthread { ... };
#else
    // 回退：基于 mutex + condition_variable 实现
    class __shared_mutex_cv { ... };
#endif
```

#### pthread_rwlock_t 版本

```cpp
class __shared_mutex_pthread {
    pthread_rwlock_t _M_rwlock = PTHREAD_RWLOCK_INITIALIZER;

public:
    // 写锁（独占）
    void lock() {
        int __ret = __glibcxx_rwlock_wrlock(&_M_rwlock);  // pthread_rwlock_wrlock
        if (__ret == EDEADLK)
            __throw_system_error(errc::resource_deadlock_would_occur);
    }

    // 读锁（共享）
    void lock_shared() {
        int __ret;
        do
            __ret = __glibcxx_rwlock_rdlock(&_M_rwlock);  // pthread_rwlock_rdlock
        while (__ret == EAGAIN);  // 读锁数量达上限重试
    }

    bool try_lock_shared() {
        int __ret = __glibcxx_rwlock_tryrdlock(&_M_rwlock);
        return !(__ret == EBUSY || __ret == EAGAIN);
    }

    void unlock_shared() {
        __glibcxx_rwlock_unlock(&_M_rwlock);  // 与 unlock() 相同
    }
};
```

**设计要点**：
- `unlock()` 和 `unlock_shared()` 都调用 `pthread_rwlock_unlock`——pthread 内部通过计数器区分读写
- 读锁超限时 `do...while` 忙等 `EAGAIN`——标准中允许实现"尽力而为"

#### condition_variable 版本 (回退实现)

当平台没有 pthread_rwlock_t 时，libstdc++ 用 `mutex + condition_variable` 模拟：

```cpp
class __shared_mutex_cv {
    mutex              _M_mut;
    condition_variable _M_gate1;   // 控制写者进入
    condition_variable _M_gate2;   // 控制写者等待读者退出
    unsigned _M_state;             // 高位=写标志，低位=读者计数

    static constexpr unsigned _S_write_entered = 1U << 31;  // 最高位
    static constexpr unsigned _S_max_readers   = ~_S_write_entered;

public:
    void lock() {               // 写锁
        unique_lock<mutex> __lk(_M_mut);
        _M_gate1.wait(__lk, [=]{ return !_M_write_entered(); });   // 无写者
        _M_state |= _S_write_entered;                                // 标记写者
        _M_gate2.wait(__lk, [=]{ return _M_readers() == 0; });      // 无读者
    }

    void lock_shared() {        // 读锁
        unique_lock<mutex> __lk(_M_mut);
        _M_gate1.wait(__lk, [=]{ return _M_state < _S_max_readers; });  // 未超限
        ++_M_state;                                                      // 读者计数++
    }
};
```

**这个实现展示了读写锁的经典算法**：
1. 写者要等**所有读者退出**和**没有其他写者**
2. 读者要等**写者完成**且**读者未超限**
3. 写者有**公平策略**：一旦写者排队，新读者被 gate1 阻止

---

### 13.8 recursive_mutex 源码分析

```cpp
class __recursive_mutex_base {
protected:
    typedef __gthread_recursive_mutex_t __native_type;  // ≡ pthread_mutex_t（特殊 attr）
#ifdef __GTHREAD_RECURSIVE_MUTEX_INIT
    __native_type _M_mutex = __GTHREAD_RECURSIVE_MUTEX_INIT;
    // → PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
};

class recursive_mutex : private __recursive_mutex_base {
public:
    void lock() {
        int __e = __gthread_recursive_mutex_lock(&_M_mutex);
        if (__e) __throw_system_error(__e);
    }

    void unlock() {
        __gthread_recursive_mutex_unlock(&_M_mutex);
    }
};
```

**不同点**：`recursive_mutex` 使用 `PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP` 初始化，这是一个设置了 `PTHREAD_MUTEX_RECURSIVE` 属性的 mutex。pthread 内部维护一个**所有者线程 ID + 递归计数**，同一线程多次 lock() 只需递增计数器，不必真的阻塞。

---

### 13.9 call_once 源码分析

#### TLS 版本（有线程局部存储）

```cpp
// /usr/include/c++/13/mutex
extern __thread void* __once_callable;   // 每个线程独有
extern __thread void (*__once_call)();   // 函数指针

struct once_flag::_Prepare_execution {
    template<typename _Callable>
    explicit _Prepare_execution(_Callable& __c) {
        __once_callable = std::__addressof(__c);   // 存 callable 地址到 TLS
        __once_call = [] {
            (*static_cast<_Callable*>(__once_callable))();  // 恢复类型后调用
        };
    }
    ~_Prepare_execution() {
        __once_callable = nullptr;
        __once_call = nullptr;
    }
};
```

#### 无 TLS 版本（回退：全局互斥 + std::function）

```cpp
extern function<void()> __once_functor;      // 全局 std::function
extern mutex& __get_once_mutex();            // 全局 mutex

struct once_flag::_Prepare_execution {
    template<typename _Callable>
    explicit _Prepare_execution(_Callable& __c) {
        __once_functor = __c;                // 存到全局 function
        __set_once_functor_lock_ptr(&_M_functor_lock);  // 持有全局 mutex 锁
    }
private:
    unique_lock<mutex> _M_functor_lock{__get_once_mutex()};
};
```

**对比**：TLS 版本无锁、零拷贝；无 TLS 版本需要全局锁和 `std::function` 的动态内存分配。现代 Linux 都支持 TLS。

---

### 13.10 pthread 类型映射总览（Linux x86-64）

| C++ 类型 | 底层 POSIX 类型 | 初始化方式 |
|----------|----------------|-----------|
| `std::thread::native_handle_type` | `pthread_t` | — |
| `std::mutex::native_handle_type` | `pthread_mutex_t*` | `PTHREAD_MUTEX_INITIALIZER` |
| `std::recursive_mutex` | `pthread_mutex_t` | `PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP` |
| `std::shared_mutex` (Linux) | `pthread_rwlock_t` | `PTHREAD_RWLOCK_INITIALIZER` |
| `std::once_flag` | `pthread_once_t` | `PTHREAD_ONCE_INIT` |
| `std::condition_variable` | `pthread_cond_t` | `PTHREAD_COND_INITIALIZER` |
| `thread_local` 变量 | 取决于链接器 | `__thread` (GCC) |

---

## 总结速查

| 概念 | 关键点 | 核心 API | 源码位置 |
|------|--------|----------|----------|
| 线程创建 | 可调用对象 + 参数 | `std::thread` | `bits/std_thread.h` |
| 生命周期 | join / detach / move | `.join()` `.detach()` `std::move` | `src/c++11/thread.cc` |
| 异常安全 | RAII 管理线程 | `ThreadGuard` / `std::jthread`(C++20) | — |
| 互斥 | 保护共享数据 | `std::mutex` + `std::lock_guard` | `bits/std_mutex.h` |
| 死锁 | 固定顺序 / std::lock | `std::lock(m1, m2)` | `include/mutex` |
| 灵活加锁 | defer / try / 超时 | `std::unique_lock` | `bits/unique_lock.h` |
| 读写分离 | 读共享、写独占 | `std::shared_mutex` + `shared_lock` | `include/shared_mutex` |
| 递归 | 同一线程重复加锁 | `std::recursive_mutex` | `include/mutex` |
| 一次性初始化 | 线程安全懒加载 | `std::call_once` + `once_flag` | `include/mutex` |
| 底层 | futex / 原子操作 / 上下文切换 | `clone()` / `LOCK CMPXCHG` | — |

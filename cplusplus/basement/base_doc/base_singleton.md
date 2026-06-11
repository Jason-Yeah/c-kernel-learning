# 单例模式（Singleton）在 C++98 → C++23 中的演进

## 什么是单例模式？

**单例模式（Singleton Pattern）** 是一种创建型设计模式，保证一个类**在整个程序生命周期中只有一个实例**，并提供一个**全局访问点**。

### 核心要点

| 要点 | 说明 |
|---|---|
| **唯一性** | 无论调用多少次，返回的都是同一个对象 |
| **全局访问** | 通过静态方法（如 `instance()`）在任何地方获取 |
| **延迟加载** | 实例在首次使用时才创建（Lazy Initialization） |
| **生命周期控制** | 实例在程序结束时自动销毁（或手动控制） |

### 典型应用场景

- **日志器（Logger）** — 全局共用一个日志输出
- **配置管理器** — 配置文件只需加载一次
- **线程池 / 连接池** — 管理有限的系统资源
- **设备驱动 / 硬件接口** — 一个物理设备只对应一个实例
- **窗口管理器 / 文件系统** — 操作系统的单一入口

### 单例 vs 全局变量

| | 单例 | 裸全局变量 |
|---|---|---|
| 唯一性保证 | 由类自己保证 | 靠程序员自觉 |
| 延迟加载 | ✅ 首次使用时创建 | ❌ 程序启动时构造 |
| 拷贝控制 | ✅ 禁止拷贝 | ❌ 可被意外复制 |
| 线程安全 | ✅ C++11 起内置 | ❌ 需额外同步 |
| 继承扩展 | ✅ 支持子类单例 | ❌ 不支持 |

---

## 1. C++98/03 — 经典单例

```cpp
class Singleton {
public:
    static Singleton* instance() {
        if (!m_instance)
            m_instance = new Singleton();
        return m_instance;
    }
private:
    Singleton() {}
    ~Singleton() {}
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);
    static Singleton* m_instance;
};

Singleton* Singleton::m_instance = nullptr;
```

### 局限

- **线程不安全** — 多个线程同时调用 `instance()` 可能创建多个实例
- **Double-Checked Locking 不可用** — C++98 无内存模型，DCLP 存在乱序问题
- **拷贝控制无法优雅 delete** — 构造函数、拷贝构造、赋值运算符只能声明为 `private`，无法用 `= delete`
- **无 `noexcept`** — 异常规范语法笨重
- **析构时机不可控** — `new` 出来的对象不会自动析构，有时需额外 `atexit` 技巧

---

## 1.5 双检锁（DCLP）—— 在 C++98 夹缝中的尝试

在 C++11 之前，为了让经典单例线程安全，人们提出了 **DCLP（Double-Checked Locking Pattern，双检锁模式）**。

### 1.5.1 原理思路

```cpp
class singleton_lazy {
public:
    static singleton_lazy* instance() {
        if (inst != nullptr)          // ① 第一次检查（无锁）
            return inst;

        _mtx.lock();                   // ② 加锁

        if (inst != nullptr)          // ③ 第二次检查（有锁）
        {
            _mtx.unlock();
            return inst;
        }

        inst = new singleton_lazy();  // ④ 创建实例
        _mtx.unlock();                // ⑤ 解锁
        return inst;
    }

private:
    static singleton_lazy* inst;
    static std::mutex _mtx;
};
```

**核心思想——两段检查：**

| 步骤 | 锁 | 目的 |
|---|---|---|
| ① 第一次 `if` | **无锁** | 实例已存在时快速返回，避免每次调用都加锁的开销 |
| ② 加锁 | 独占 | 确保只有一个线程进入临界区 |
| ③ 第二次 `if` | **有锁** | 防止"两个线程同时通过①，一个等另一个解锁后重复创建" |
| ④ `new` | 有锁 | 在锁保护下安全创建 |
| ⑤ 解锁 | — | 释放锁 |

**为什么需要两次检查？**

```
线程A: ① inst==nullptr → 通过 → ② 加锁 → ③ inst==nullptr → 通过 → ④ new ...
线程B:                         等待锁...
                                                      线程B拿到锁 → ③ inst≠nullptr → 返回已有实例 ✅
```

如果没有第二次检查，线程B拿到锁后会直接再次 `new`，破坏单例唯一性。

### 1.5.2 DCLP 在 C++98 下的致命缺陷

**DCLP 在 C++98 中是错误的，原因在于指令重排（Instruction Reordering）。**

问题出在第 ④ 步 `inst = new singleton_lazy();`，这条语句在底层分解为三步：

```
步骤 A:  分配内存              — operator new(sizeof(singleton_lazy))
步骤 B:  在内存上构造对象       — 调用构造函数
步骤 C:  将地址赋给 inst       — inst = 分配到的地址
```

**编译器或 CPU 可能将步骤 C 重排到步骤 B 之前！**

```
实际执行顺序可能变成： A → C → B

线程A:  ① inst==nullptr → 通过 → ② 加锁 → ③ inst==nullptr → 通过
         → A(分配内存) → C(inst = 地址，此时 inst≠nullptr，但对象尚未构造!)
         → 此时线程B 调用 instance()
线程B:  ① inst≠nullptr → ❌ 直接返回 inst → 访问未构造完成的对象 → **未定义行为!**
```

这种 bug 极难复现，只在多核 CPU 上偶发，是典型的**竞态条件（Race Condition）**。

### 1.5.3 C++11 的修复方案

C++11 引入了**内存模型 + `std::atomic`**，通过内存序（memory order）禁止重排：

```cpp
#include <atomic>
#include <mutex>

class singleton_lazy {
public:
    static singleton_lazy* instance() {
        singleton_lazy* p = inst.load(std::memory_order_acquire);   // ① acquire: 之后的读不能重排到前面
        if (!p) {
            std::lock_guard<std::mutex> lock(mtx);
            p = inst.load(std::memory_order_relaxed);               // ③ 在锁保护下，relaxed 就够
            if (!p) {
                p = new singleton_lazy();
                inst.store(p, std::memory_order_release);           // ④ release: 之前的写不能重排到后面
            }
        }
        return p;
    }

private:
    singleton_lazy() = default;

    static std::atomic<singleton_lazy*> inst;
    static std::mutex mtx;
};
```

**`acquire` / `release` 的作用：**

| 内存序 | 位置 | 效果 |
|---|---|---|
| `memory_order_acquire` | ① `load` | 禁止之后的读写（包括解引用实例）重排到 `load` 之前 |
| `memory_order_release` | ④ `store` | 禁止之前的操作（包括构造函数）重排到 `store` 之后 |

`acquire-release` 配对形成**happens-before**关系：
- 线程B 的 `acquire load` 看到线程A 的 `release store` → 线程B 必定看到线程A 在 `store` 之前的所有操作（包括构造完成的对象）

> **注意：** `singleton_lazy` 中直接用裸 `static singleton_lazy* inst` + `std::mutex` 的写法，在 C++98/11 下仍然有数据竞争（非原子变量的读写不同步），属于未定义行为。**正确的 DCLP 必须使用 `std::atomic`。**

### 1.5.4 最佳实践：别自己写 DCLP

既然 C++11 的静态局部变量已经线程安全，**不要再手动实现 DCLP**：

| 方案 | 代码量 | 正确性 | 性能 |
|---|---|---|---|
| Meyer's Singleton | 1 行 `static T inst;` | ✅ 标准保证 | 最优（C++11 编译器通常用更高效的实现） |
| `std::call_once` | 3 行 | ✅ 标准保证 | 略逊于 Meyer's |
| 手写 `std::atomic` DCLP | 10+ 行 | ⚠️ 容易写错 | 与 Meyer's 接近 |
| C++98 裸指针 DCLP | 10+ 行 | ❌ 有 bug | — |

**结论：** 理解 DCLP 是为了看懂历史代码和面试，**生产代码直接用 Meyer's Singleton**。

---

## 2. C++11 — 质变：线程安全内置 + 现代语法

### 2.1 静态局部变量

```cpp
class Singleton {
public:
    static Singleton& instance() {
        static Singleton inst;   // C++11: 线程安全的首次初始化 (§6.7/4)
        return inst;
    }

    Singleton(const Singleton&)            = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&)                 = delete;
    Singleton& operator=(Singleton&&)      = delete;

private:
    Singleton() noexcept = default;
    ~Singleton()         = default;
};
```

| 改进 | 说明 |
|---|---|
| **线程安全** | C++11 保证静态局部变量首次初始化的线程安全性，Meyer's Singleton 无需额外锁 |
| **`= delete`** | 优雅禁止拷贝/移动 |
| **`noexcept`** | 明确异常保证 |
| **析构可控** | 局部 static 对象在 `main()` 结束后自动析构 |

### 2.2 `std::call_once` + `std::once_flag`

```cpp
class Singleton {
public:
    static Singleton& instance() {
        std::call_once(flag, [] { m_instance.reset(new Singleton()); });
        return *m_instance;
    }
private:
    Singleton() = default;
    static std::unique_ptr<Singleton> m_instance;
    static std::once_flag flag;
};
```

### 2.3 `std::atomic` — 正确的 DCLP

C++11 引入了**内存序（memory order）**，有了内存模型后 DCLP 才能正确实现：

```cpp
class Singleton {
public:
    static Singleton* instance() {
        Singleton* p = m_instance.load(std::memory_order_acquire);
        if (!p) {
            std::lock_guard<std::mutex> lock(m_mutex);
            p = m_instance.load(std::memory_order_relaxed);
            if (!p) {
                p = new Singleton();
                m_instance.store(p, std::memory_order_release);
            }
        }
        return p;
    }
private:
    static std::atomic<Singleton*> m_instance;
    static std::mutex m_mutex;
};
```

### 2.4 `constexpr` — 编译期单例雏形

```cpp
struct Config {
    int value = 42;
};
constexpr Config global_config;  // C++11 constexpr
```

---

## 3. C++14/17 — 丰富与简化

### 3.1 `inline` 变量 (C++17)

```cpp
// singleton.h — 头文件即可，无需单独 .cpp
class Singleton {
public:
    static Singleton& instance() {
        static Singleton inst;
        return inst;
    }
};

inline Singleton& g_singleton = Singleton::instance();
// ^^ inline 保证跨 TU 只有一个副本
```

### 3.2 `std::optional` (C++17) — 无堆分配懒加载

```cpp
class Singleton {
public:
    static Singleton& instance() {
        static std::optional<Singleton> inst;
        if (!inst)
            inst.emplace();
        return *inst;
    }
};
```

### 3.3 `std::shared_mutex` (C++17) — 读写锁

适用于包含全局注册表/缓存的单例，读多写少场景：

```cpp
class Registry {
public:
    void register_service(const std::string& name, Service* svc) {
        std::unique_lock lock(m_mutex);
        m_services[name] = svc;
    }
    Service* lookup(const std::string& name) {
        std::shared_lock lock(m_mutex);   // 读共享
        auto it = m_services.find(name);
        return it != m_services.end() ? it->second : nullptr;
    }
    // ...
};
```

### 3.4 `if constexpr` (C++17) — 模板化单例策略

```cpp
template <typename T, bool ThreadSafe = true>
class Singleton {
public:
    static T& instance() {
        if constexpr (ThreadSafe) {
            std::call_once(flag, [] { m_instance = std::make_unique<T>(); });
        } else {
            if (!m_instance)
                m_instance = std::make_unique<T>();
        }
        return *m_instance;
    }
};
```

---

## 4. C++20 — 可重入与编译期保障

### 4.1 `constinit` — 静态初始化顺序保证

```cpp
struct Config {
    int version = 1;
};

// constinit 保证在 dynamic initialization 之前完成静态初始化
// 避免 "static initialization order fiasco"
constinit Config global_config;
```

### 4.2 `consteval` — 纯编译期单例

```cpp
struct SingletonBuffer {
    char buffer[1024]{};
};
consteval SingletonBuffer make_buffer() {
    return SingletonBuffer{};
}
inline constexpr auto g_buffer = make_buffer();
```

### 4.3 CRTP 单例基类

**CRTP（Curiously Recurring Template Pattern，奇异递归模板模式）** 是 C++ 模板元编程中的一种经典技巧：派生类将自己作为模板参数传给基类。

```cpp
template <typename Derived>
class Base {
    // 基类能通过 static_cast<Derived*>(this) 调用派生类的接口
};

class Derived : public Base<Derived> {
    // ...
};
```

#### 4.3.1 原理

CRTP 本质上是**通过模板在编译期实现静态多态**，避免了虚函数的运行时开销：

| | 虚函数多态 | CRTP 静态多态 |
|---|---|---|
| 绑定时机 | **运行时**（vtable） | **编译期**（模板实例化） |
| 调用开销 | 间接跳转（vptr 查表） | 直接调用（可内联） |
| 类型擦除 | ✅ 通过基类指针存不同类型 | ❌ 每种派生类是不同的独立类型 |
| 代码膨胀 | 一份虚表 | 每实例化一个派生类生成一份代码 |

#### 4.3.2 CRTP 实现单例基类

```cpp
template <typename Derived>
class SingletonBase {
public:
    SingletonBase(const SingletonBase&)            = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    SingletonBase(SingletonBase&&)                 = delete;
    SingletonBase& operator=(SingletonBase&&)      = delete;

    [[nodiscard]] static Derived& instance() {
        static Derived inst;               // C++11 线程安全
        return inst;
    }

protected:
    SingletonBase() noexcept = default;
    ~SingletonBase()         = default;    // 防止外部 delete 基类指针
};

class Logger : public SingletonBase<Logger> {
    friend class SingletonBase<Logger>;    // 基类调用 private 构造需要友元
public:
    void log(std::string_view msg) { /* ... */ }
private:
    Logger() = default;                    // 禁止外部构造
};
```

**各要素的作用：**

| 组件 | 作用 |
|---|---|
| `SingletonBase<Logger>` | 每个派生类实例化一份独立的 `instance()` 代码 |
| `friend class SingletonBase<Logger>` | 基类的 `instance()` 才能调用 `Logger` 的私有构造 |
| `static Derived inst` | Meyer's Singleton，线程安全 |
| `protected 构造/析构` | 防止外部直接创建基类，但派生类可以继承 |
| `= delete` 拷贝/移动 | 禁止复制单例 |

#### 4.3.3 多单例统一管理

CRTP + 注册表可以实现**所有单例的统一生命周期管理**：

```cpp
class SingletonRegistry {
public:
    static void cleanup_all() {
        for (auto& dtor : destructors())
            dtor();
    }
protected:
    static void register_destructor(std::function<void()> dtor) {
        destructors().push_back(std::move(dtor));
    }
private:
    static std::vector<std::function<void()>>& destructors() {
        static std::vector<std::function<void()>> inst;
        return inst;
    }
};

template <typename Derived>
class ManagedSingleton : public SingletonRegistry {
public:
    static Derived& instance() {
        static Derived inst;
        static bool registered = (register_destructor([]{ /* 自定义清理 */ }), true);
        (void)registered;
        return inst;
    }
};
```

#### 4.3.4 CRTP 的优缺点

| 优点 | 缺点 |
|---|---|
| ✅ **零开销抽象** — 比虚函数高效 | ❌ **代码膨胀** — 每实例化一次生成一份代码 |
| ✅ **编译期多态** — 错误在编译期捕获 | ❌ **类型独立** — 无法通过基类指针统一管理 |
| ✅ **可复用** — 一行 `: public SingletonBase<T>` 获得单例能力 | ❌ **友元声明啰嗦** — 每个派生类都要声明 friend |
| ✅ **组合灵活** — 可叠加其他 CRTP（如 `Comparable<T>`） | ❌ **学习曲线** — 不如继承直观，新手难读 |

---

## 5. C++23 — 展望

### 5.1 `std::optional` + `std::monostate` / `std::expected`

单例返回可带错误状态，初始化失败时不再终止程序。

### 5.2 显式生命周期管理

`std::start_lifetime_as` 等新设施使 placement new 单例更安全。

### 5.3 `std::out_ptr` / `std::inout_ptr`

与 C 库互操作时管理单例资源生命周期更简洁。

---

## 总结对比

| 标准 | 关键变化 | 推荐的单例写法 |
|---|---|---|
| C++98/03 | 经典 DCLP（有 bug），无线程安全 | `new Singleton()` + 手动锁 |
| **C++11** | 静态局部变量线程安全、`= delete`、`std::call_once`、内存模型 | **Meyer's Singleton 成为默认选择** |
| C++14 | 无重大变化 | 同 C++11 |
| C++17 | `inline` 变量、`std::optional`、`if constexpr`、`std::shared_mutex` | 头文件内联单例、模板策略 |
| **C++20** | `constinit`、`consteval`、CRTP 现代化 | constinit 全局 + Meyer's 组合 |
| C++23 | 生命周期工具、`std::expected` | 更安全的资源管理 |

**结论：** C++11 是单例模式的分水岭——静态局部变量的线程安全保证让 Meyer's Singleton 成为当今最简洁、最推荐的写法。

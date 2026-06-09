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

### 4.3 CRTP + Mixin 模式

```cpp
template <typename Derived>
class SingletonBase {
public:
    SingletonBase(const SingletonBase&)            = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;

    [[nodiscard]] static Derived& instance() {
        static Derived inst;
        return inst;
    }

protected:
    SingletonBase() noexcept = default;
    ~SingletonBase()         = default;
};

class Logger : public SingletonBase<Logger> {
    friend class SingletonBase<Logger>;
public:
    void log(std::string_view msg) { /* ... */ }
private:
    Logger() = default;
};
```

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

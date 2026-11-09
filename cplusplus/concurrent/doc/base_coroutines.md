# 从零开始写协程：C / C++ 的分步编程教程

这篇文档不要求你已经懂协程语法。目标不是先记住一堆名词，而是按顺序完成几件事：**写出程序 → 编译运行 → 看懂暂停与恢复 → 写异步业务 → 再理解底层实现。**

配套源码与依赖说明：[base_coroutines/readme.md](../code/base_coroutines/readme.md)。本文是独立专题，不是某本书的章节摘要。

## 学习地图：先会用，再研究实现

| 步骤 | 你要做的事 | 学完能回答什么 |
|---|---|---|
| 第 0 步 | 选择 C / C++ 路线，准备编译环境 | 我的语言版本能不能直接写协程？ |
| 第 1 步 | 写一个最小 C++20 协程并手动恢复 | `co_await`、`co_return` 怎么写？ |
| 第 2 步 | 给程序增加暂停点、交替推进两个任务 | 谁决定什么时候继续？局部变量还在吗？ |
| 第 3 步 | 对照 C / C++11 的手工状态机 | 编译器替我保存了什么？ |
| 第 4 步 | 使用库写异步定时任务 | 实际开发怎么启动任务、等待、取得结果？ |
| 第 5 步 | 阅读并运行 TCP 回显服务 | 怎么接收连接、读写、处理错误？ |
| 第 6 步 | 学习 `co_yield` 生成器 | 如何按需产生一串数据？ |
| 第 7 步 | 理解 Task / promise / awaiter / epoll | 暂停以后，到底是谁恢复协程？ |
| 第 8 步 | 检查生命周期、取消、阻塞与性能 | 从示例走向项目，还缺哪些保证？ |

**建议按步骤学习，不要先从第 7 步的底层协议开始。** 第 1～2 步可暂时把配套 `Task` 当成现成工具；实际业务通常也会使用库提供的工具，而不是从零写协程运行时。

---

## 第 0 步：先选路线，准备环境

### 0.1 C 和 C++ 的协程写法不是同一回事

| 你在用什么 | 能怎么做 | 本文建议 |
|---|---|---|
| 纯 C（包括 C11 / C17 / C23） | 标准没有 `co_await` 等协程关键字；可手写状态机，或使用第三方协程库 | 第 3 步先写出状态机；需要真正上下文切换时再选择库 |
| C++11 / C++14 / C++17 | 标准尚无 C++20 协程语法；可用状态机、回调或库提供的有栈协程 | 运行配套示例 01，再根据现有工程选择库 |
| C++20 及以后 | 可使用 `co_await`、`co_yield`、`co_return` 与 `<coroutine>` | 按本文主线学习 |

一些旧编译器提供协程扩展，但本教程不依赖它们。**不能仅把 C 文件改名为 `.cpp`，就认为已有了协程运行时。**

C++20 提供语言机制，但没有标准的通用 `std::task`、网络事件循环或异步 socket 库。后面用到的 `Task` 是仓库的教学类型，`asio::awaitable` 则来自 Boost.Asio。二者不能随意混用。

### 0.2 你需要哪些普通 C++ 基础

只需先认识：函数、局部变量、`if` / `for`、类与对象、作用域、引用和 `std::string`。模板、移动语义和异常如果还不熟，可以先按旁边的说明使用。

先把三个概念说清楚：

- **线程**：操作系统安排执行的执行流，有自己的执行栈。
- **协程**：可以在中间暂停、以后接着执行的函数式任务；它仍然运行在线程上。
- **调度**：决定这次推进哪个任务。最简单的调度，就是 `main` 手动调用它的 `resume()`。

这里的暂停不是“整个程序停住”。例如 A 暂停后，线程可以回到 `main`，再执行 B。

### 0.3 进入配套目录并检查工具

下面的命令在 Linux 终端执行，不是写进 C++ 文件：

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_coroutines
g++ --version
make
```

需要编译器和标准库支持 C++20 的 `<coroutine>`。配套默认示例已用 G++ 13.3.0 验证；不要只依靠版本号判断，实际编译是否成功才是依据。

- `g++`：C++ 编译器。
- `-std=c++20`：启用 C++20 语言标准。
- `make`：按目录里的 Makefile 编译前四个程序；不默认编译依赖 Boost 的示例。
- `./程序名`：执行当前目录下的程序。

**本节完成标志：** `make` 成功。暂时不需要读 epoll、promise 或全部源码。

## 第 1 步：亲手写第一个 C++20 协程

### 1.1 先使用现成的 Task，不急着实现它

在刚才的配套目录新建 `learn_manual.cpp`，输入下面的完整程序。它依赖同目录已有的 [task.hpp](../code/base_coroutines/task.hpp)，不是单独一个文件就能放到任意目录编译的程序。

```cpp
#include "task.hpp"  // 本仓库提供的 Task，内部已包含 <coroutine>
#include <iostream>

Task count_twice()
{
    int value = 10;
    std::cout << "task: " << value << '\n';

    co_await std::suspend_always{};

    ++value;
    std::cout << "task: " << value << '\n';
    co_return;
}

int main()
{
    auto task = count_twice();
    std::cout << "main: created\n";

    task.resume();
    std::cout << "main: between\n";

    task.resume();
    std::cout << "main: done=" << task.done() << '\n';
}
```

编译运行：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic learn_manual.cpp -o learn_manual
./learn_manual
```

预期输出：

```text
main: created
task: 10
main: between
task: 11
main: done=1
```

这里 `1` 表示 `true`，即任务已完成。程序只有一个线程，没有后台线程。

### 1.2 逐句读懂新语法

| 代码 | 怎么读 |
|---|---|
| `Task count_twice()` | 定义一个无参数函数，返回本例的任务对象 `Task`，而不是 `void` |
| `auto task = count_twice();` | 调用函数取得任务对象；`auto` 让编译器推导变量类型，这里就是 `Task` |
| `task.resume();` | 调用对象的方法，推进这个协程，直到它再次暂停或完成 |
| `co_await std::suspend_always{};` | 在这里暂停，等待外部以后恢复；这里没有任何实际 I/O 操作 |
| `co_return;` | 结束协程，不返回业务数值 |
| `task.done()` | 查询本例任务是否已完成 |

`std::suspend_always{}` 也可以拆开看：

- `std::`：这个类型属于标准库命名空间。
- `suspend_always`：一个标准类型，作用是“等待它时总要暂停”。
- `{}`：构造这个类型的一个对象；不是函数体。
- `co_await 对象;`：使用这个对象规定的等待规则。

**不要机械地把普通函数中的 `return` 全部改成 `co_return`。** 一个使用这些协程关键字的函数还需要合适的返回类型及协议；普通 `int` / `void` 不能直接替代这里的 `Task`。

### 1.3 按时间顺序走一遍

| 操作 | 协程执行到哪里 | `value` | 执行权回到哪里 |
|---|---|---|---|
| `count_twice()` | 本例先停在函数体执行之前 | 尚未执行初始化 | `main` |
| 第一次 `task.resume()` | 输出 10，停在 `co_await` | 10，仍保存着 | `main`，输出 between |
| 第二次 `task.resume()` | 从暂停处继续，执行 `++value`，输出 11，结束 | 正常结束作用域 | `main` |
| `main` 结束 | Task 析构，回收协程状态 | 不再存在 | 程序结束 |

这里“创建时不执行函数体”是 **本例 Task 的策略**，不是所有协程类型的统一规则。

普通函数返回后，这次调用的局部对象通常就结束生命周期；这里第一次暂停后，`value` 仍然存在。编译器保存了继续执行需要的状态，所以第二次不会重做 `int value = 10`。

### 1.4 这还不是异步网络程序

这个程序只演示暂停与恢复：

```text
main → 恢复任务 → 任务输出 10 并暂停 → main 做别的事
main → 恢复任务 → 任务输出 11 并完成 → main
```

`std::suspend_always` 不会启动定时器，不会注册 socket，也不会自动安排下一次执行。**如果 `main` 不再恢复任务，它就不会自己继续。**

**本节练习：** 先删去第二次 `task.resume()`，预测再运行。你应看到只有 `task: 10`，且 `done=0`。Task 析构会清理暂停中的状态，不会替你执行剩下的输出。

## 第 2 步：学会控制暂停点与多个任务

### 2.1 自己增加一个阶段

把刚才的 `count_twice()` 替换为以下函数，其他代码先不动：

```cpp
Task count_twice()
{
    int value = 10;
    std::cout << "task: " << value << '\n';
    co_await std::suspend_always{};

    ++value;
    std::cout << "task: " << value << '\n';
    co_await std::suspend_always{};

    ++value;
    std::cout << "task: " << value << '\n';
    co_return;
}
```

这次两次恢复只会输出 10、11，任务还没完成。你需要在 `main` 中增加第三次 `task.resume()`，才能输出 12 并完成。

这个练习建立一个重要习惯：**先指出暂停点，再说明谁会恢复它。**

这里只是人为暂停，因此可以手动恢复；后面的网络程序必须等对应事件发生，不能用一个忙循环无条件反复 `resume()`。

### 2.2 交替推进 A、B 两个任务

现在打开并运行配套 [02_manual_resume_cpp20.cpp](../code/base_coroutines/02_manual_resume_cpp20.cpp)：

```bash
./manual_resume
```

核心协程如下，`std::string name` 是按值传入的名字，每个任务拥有自己的参数副本：

```cpp
Task conversation(std::string name)
{
    int value = 10;
    std::cout << name << ": first, value=" << value << '\n';
    co_await std::suspend_always{};
    ++value;
    std::cout << name << ": second, value=" << value << '\n';
    co_return;
}
```

调用方的关键操作是：

```cpp
auto a = conversation("A");
auto b = conversation("B");
a.resume();
b.resume();
a.resume();
b.resume();
```

对应输出的业务部分：

```text
A: first, value=10
B: first, value=10
A: second, value=11
B: second, value=11
```

两份任务各有自己的 `value`。A 暂停并不意味着 B 自动运行，是 `main` 接下来调用了 `b.resume()`。

这叫**并发**：多个任务交错推进。它不是**并行**：同一时刻在多个 CPU 核心上执行。要多核计算，仍需线程或进程；协程可以与它们配合。

### 2.3 第一次认识三个关键字

| 关键字 | 先记住的含义 | 后面在哪里用 |
|---|---|---|
| `co_await operation` | 按 operation 的规则等待，可能暂停，也可能立即继续 | 定时器、socket 异步操作 |
| `co_return` / `co_return value` | 完成协程，可按返回类型的约定交付一个值 | 任务结束、异步计算结果 |
| `co_yield value` | 产出一个值；常见生成器会随后暂停 | 第 6 步的逐项生成 |

不是每个 `co_await` 都暂停。例如把本节的 `std::suspend_always{}` 换成 `std::suspend_never{}`，第一次恢复就会连续输出两个阶段并完成。

本例 Task 的 `resume()` 会检查完成状态，避免再次恢复；**原始 `coroutine_handle` 并没有这种保护，不能恢复最终挂起的协程。**

**本节完成标志：** 你可以不运行程序，就预测“删去 / 增加一个恢复调用”对输出的影响。

## 第 3 步：没有协程关键字时，C / C++ 怎么做

这一节用于理解“保存进度”。如果你主要学 C++20，完成比较后继续第 4 步；不必先实现自己的协程库。

### 3.1 先认识状态机，不需要额外库

假设任务要分两次输出 10、11。普通函数调用两次，默认每次都从入口开始；想接着上次做，就需要保存：

1. **数据**：当前数值是多少。
2. **进度**：下一次执行第一阶段、第二阶段，还是已经完成。

把进度放到变量，用 `if` / `switch` 根据进度选择一段代码，就是这里的**显式状态机**。“显式”是程序员明确写出来，不是显示到屏幕上。

### 3.2 纯 C：完整写一个可分步推进的任务

在配套目录新建 `learn_fsm.c`：

```c
#include <stdio.h>

typedef enum {
    STEP_FIRST,
    STEP_SECOND,
    STEP_DONE
} Step;

typedef struct {
    const char *name;
    Step step;
    int value;
} Conversation;

void conversation_resume(Conversation *task)
{
    switch (task->step) {
    case STEP_FIRST:
        task->value = 10;
        printf("%s: first, value=%d\n", task->name, task->value);
        task->step = STEP_SECOND;
        return;
    case STEP_SECOND:
        ++task->value;
        printf("%s: second, value=%d\n", task->name, task->value);
        task->step = STEP_DONE;
        return;
    case STEP_DONE:
        return;
    }
}

int main(void)
{
    Conversation a = {"A", STEP_FIRST, 0};
    Conversation b = {"B", STEP_FIRST, 0};
    while (a.step != STEP_DONE || b.step != STEP_DONE) {
        conversation_resume(&a);
        conversation_resume(&b);
    }
    return 0;
}
```

编译运行：

```bash
cc -std=c11 -Wall -Wextra -Wpedantic learn_fsm.c -o learn_fsm
./learn_fsm
```

输出与上一节 A/B 的四行输出相同。

新语法解释：

- `enum` 给状态起名字；`STEP_SECOND` 表示“下一次该执行第二阶段”。
- `struct` 把一份任务需要保存的东西放在一起。
- `&a` 取得对象地址；函数里的 `task->value` 访问该地址所指对象的成员。
- `task->step = STEP_SECOND` 只修改状态，不会立即跳到另一个 `case`；紧接着 `return` 回到调用方。
- `a`、`b` 一直活在 `main` 的作用域中，所以成员数据不会随着 `conversation_resume()` 返回而消失。

**这不是语言级协程，也不是有独立栈的协程。** 每次函数仍从入口执行，只是根据保存的状态选择不同分支。它展示了协程要解决的核心问题。

这个循环仅适合本例“每次都能推进”的任务。如果任务在等网络数据，就应该等事件，不能原地反复调用消耗 CPU。

### 3.3 C++11：把结构体与操作封装成类

运行 [01_manual_fsm_cpp11.cpp](../code/base_coroutines/01_manual_fsm_cpp11.cpp)：

```bash
./manual_fsm
```

它用 `Conversation` 类保存 `state_`、`value_`，用 `resume()` 推进。成员末尾的 `_` 只是命名习惯，不是特殊语法。`enum class State` 是 C++11 的有作用域枚举，比如用 `State::second` 表示其中一个状态。

| 手工 C / C++ 状态机 | 第 1 步的 C++20 协程 |
|---|---|
| 自己存 step / state_ | 编译器保存执行位置 |
| 自己把跨阶段的数据放在结构体 / 类成员里 | 编译器保存跨暂停点仍需要的数据 |
| `switch` 选阶段，再用普通 `return` 返回 | `co_await` 挂起，以后继续这次逻辑执行 |
| 调用方决定何时再调用推进函数 | 调用方或运行库决定何时恢复协程 |

### 3.4 如果纯 C 项目确实需要“有栈协程”

有栈协程通常为任务准备独立栈，由库保存 / 恢复执行上下文。它与上述手工状态机、C++20 无栈协程是不同路线。

仓库的 [ucontext 示例](../../basement/base_code/src/coroutine_demo.cpp) 可帮助观察 `swapcontext` 如何切换，但它是 C++ 文件且使用平台 API，不能当成可移植的 ISO C / C++ 标准能力。`ucontext` 接口也已从 POSIX.1-2008 移除，不建议直接据此设计新的跨平台运行时。

实际 C 项目应先选择明确支持目标平台的库，然后按该库文档完成：**初始化运行时 → 创建任务及栈 → 调度 / 让出 → 退出并回收**。函数名、栈管理、I/O 接入都随库而变，没有统一的 C 协程语法。不要把 `setjmp` / `longjmp` 当成可以随意恢复已退出函数栈的通用方案。

C++11～17 的现有工程也可以考虑 Boost.Asio `spawn` 等库提供的有栈协程，需按实际 Boost 版本配置相应依赖。参见 [Asio spawn 文档](https://www.boost.org/doc/libs/latest/doc/html/boost_asio/reference/spawn.html)。

## 第 4 步：走向实际开发——用库写异步定时任务

手动 `resume()` 是为了看清执行过程。实际业务通常让运行库根据 I/O 或定时器事件恢复任务。

下面选择 **Boost.Asio + C++20** 作为开发路线：先做一个不涉及网络的定时器，再把同一套结构用于 TCP。初学时不建议先从零写通用调度器。

### 4.1 准备 Boost，并认识四个名字

这一节及下一节额外依赖 Boost.Asio。当前检查环境未安装 Boost 头文件，下面的可选示例未在本次环境编译运行；默认四个仓库示例不受影响。

Ubuntu / Debian 可以按需要安装：

```bash
sudo apt install libboost-dev
```

其他系统使用自己的包管理器。老版本 Boost 或工具链不一定支持下面接口，需要核对 [Asio C++20 支持说明](https://www.boost.org/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html)。

| 名字 | 业务作者如何理解 |
|---|---|
| `asio::awaitable<T>` | Asio 提供的协程返回类型；`T` 是完成后交付的结果类型 |
| `asio::use_awaitable` | 告诉异步 API：用可 `co_await` 的方式交付结果，而不是让我传普通回调 |
| `asio::co_spawn(...)` | 把任务交给 Asio 执行，并指定完成时如何处理 |
| `asio::io_context` 与 `io.run()` | 事件处理环境，以及驱动事件处理的循环 |

先不要研究这些类型的内部实现。业务编程先掌握调用顺序即可。

### 4.2 写出完整的异步定时器程序

在配套目录新建 `learn_timer.cpp`：

```cpp
#include <utility>
#include <boost/asio.hpp>
#include <chrono>
#include <exception>
#include <iostream>
#include <string>

namespace asio = boost::asio;

asio::awaitable<void> delayed_print(std::string name, int milliseconds)
{
    auto executor = co_await asio::this_coro::executor;
    asio::steady_timer timer(executor);
    timer.expires_after(std::chrono::milliseconds(milliseconds));

    std::cout << name << ": waiting\n";
    co_await timer.async_wait(asio::use_awaitable);
    std::cout << name << ": finished\n";
    co_return;
}

void on_complete(std::exception_ptr error)
{
    if (!error)
        return;
    try {
        std::rethrow_exception(error);
    } catch (const std::exception &e) {
        std::cerr << "task failed: " << e.what() << '\n';
    }
}

int main()
{
    asio::io_context io;
    asio::co_spawn(io, delayed_print("A", 80), on_complete);
    asio::co_spawn(io, delayed_print("B", 30), on_complete);
    io.run();
}
```

编译运行（需要兼容的 Boost 和工具链）：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic \
    -DBOOST_ERROR_CODE_HEADER_ONLY learn_timer.cpp -pthread -o learn_timer
./learn_timer
```

正常负载下通常看到：

```text
A: waiting
B: waiting
B: finished
A: finished
```

定时器不是精确实时保证，启动和恢复顺序也不应作为业务正确性的前提。

### 4.3 一行一行理解业务写法

1. `namespace asio = boost::asio;`：给命名空间起短名字，不是创建对象。
2. `asio::awaitable<void>`：这是 Asio 协程；它完成后不交付业务数值。这里不使用教学 `Task`。
3. `co_await asio::this_coro::executor`：取得当前协程使用的执行器，即任务在什么执行环境中运行；这不是等待网络。
4. `steady_timer timer(executor)`：创建属于该执行环境的定时器。局部变量在等待期间仍存活。
5. `expires_after(...)`：设置从现在起多久到期；这行本身不阻塞线程睡眠。
6. `co_await timer.async_wait(asio::use_awaitable)`：启动异步等待；未完成时暂停当前协程，到期后由 Asio 恢复。
7. `co_spawn(...)`：提交两个任务；不代表创建两个 OS 线程。
8. `io.run()`：当前线程驱动事件处理；有可处理工作就执行，没有就等待，直到没有剩余工作或被停止。
9. `on_complete`：任务完成后观察异常；`exception_ptr` 用来保存异常，`rethrow_exception` 将它重新抛出以便处理。

只有一个线程调用 `io.run()`，所以 A、B 的代码不在两个核心上同时运行。等待 A 时，这个线程有机会处理 B；都在等时，事件循环可以等待事件，而不是不断空转。

### 4.4 两种“等待”不能混用

```cpp
// 错误方向：即使写在协程中，也会阻塞当前线程。
std::this_thread::sleep_for(std::chrono::seconds(1));

// Asio 业务协程中的正确方向：异步等待，必要时仅暂停当前协程。
co_await timer.async_wait(asio::use_awaitable);
```

上面的片段用于比较行为，不是独立程序。同步 `sleep_for` 返回 `void`，也不能在它前面加 `co_await` 就变成异步 API。

### 4.5 如何调用另一个协程并取得结果

实际开发很常见的形式是：“等一个子任务完成，拿到值，再继续”。以下是可加入 `learn_timer.cpp`、放在 `main` 前的两个函数：

```cpp
asio::awaitable<int> calculate()
{
    co_return 42;
}

asio::awaitable<void> caller()
{
    int result = co_await calculate();
    std::cout << "result=" << result << '\n';
    co_return;
}
```

在 `main` 的 `io.run()` **之前**增加：

```cpp
asio::co_spawn(io, caller(), on_complete);
```

你会额外看到 `result=42`，不必假设它与其他输出的相对顺序。这里没有慢操作，目的是演示语法。

需要分清三件事：

- `calculate()` 返回的是 `asio::awaitable<int>`，不是立即返回整数。
- 子协程中的 `co_return 42` 交付最终结果。
- 父协程中 `co_await calculate()` 的表达式结果才是整数 42。异常也可以沿着等待关系传播。

本仓库教学 `Task` **没有实现可等待协议，不可写 `co_await 某个Task`**；不要拿这段 Asio 组合方式直接套用它。标准 `std::future` 在 C++20 中也没有自动变得可 `co_await`。

**本节练习：** 增加 C，等待 10 ms；再把 A/B 延时互换。解释为什么完成顺序取决于事件，而不只是创建顺序。

## 第 5 步：按同样的方法开发一个 TCP 回显服务

定时器和网络协程的业务骨架相同：创建运行环境、编写返回 `awaitable` 的函数、等待异步操作、启动任务、驱动事件循环。

### 5.1 先运行完整程序，不要只复制局部片段

完整代码在 [05_asio_echo_cpp20.cpp](../code/base_coroutines/05_asio_echo_cpp20.cpp)。按第 4 步准备好 Boost 后，在配套目录执行：

```bash
make asio_echo
./asio_echo 9020
```

程序只监听 `127.0.0.1:9020`。如果提示端口被占用，可以换一个可用端口，并同步修改客户端端口。

另开终端，用 Python 标准库测试：

```bash
python3 - <<'PY'
import socket
with socket.create_connection(('127.0.0.1', 9020), timeout=3) as s:
    s.sendall(b'hello coroutine\n')
    s.shutdown(socket.SHUT_WR)
    chunks = []
    while True:
        data = s.recv(4096)
        if not data:
            break
        chunks.append(data)
    print(b''.join(chunks).decode(), end='')
PY
```

客户端预期收到 `hello coroutine`。`shutdown(SHUT_WR)` 表示不再发送，让服务端最终读到 EOF；客户端仍能接收回显。

### 5.2 第一个业务协程：处理一条连接

下面是完整文件中会话函数的**核心片段**，省略了外层异常处理，不可单独编译：

```cpp
asio::awaitable<void> echo_session(tcp::socket socket)
{
    std::array<char, 4096> buffer{};
    for (;;)
    {
        std::size_t n = co_await socket.async_read_some(
            asio::buffer(buffer), asio::use_awaitable);

        co_await asio::async_write(
            socket, asio::buffer(buffer.data(), n), asio::use_awaitable);
    }
}
```

按业务顺序解释：

1. `tcp::socket socket` 按值接收连接，调用方通过移动把所有权交给会话。
2. `std::array<char, 4096>` 是固定容量数组，属于这次会话的局部状态。
3. `async_read_some` 开始异步读取；没完成时允许暂停该会话，而不是卡住整个事件线程。
4. 恢复后，`n` 是本次实际读到的字节数，**不是缓冲区容量，也不一定是一条完整消息**。
5. `asio::buffer` 只是描述内存范围，不拥有这块内存；数组必须活到操作完成。
6. `async_write` 是组合操作，处理部分写入，成功时写完指定的 n 个字节。
7. 写完再读，形成顺序业务流程。完整源码将 EOF 作为正常断开处理，其他错误继续抛给完成处理器。

TCP 是字节流。“一次读到的数据”不等于“一条请求”。回显服务只照样写回字节，可以不解析消息；聊天协议、HTTP 或数据库请求则需要长度字段、分隔符等分帧规则。

### 5.3 第二个业务协程：接收新连接并启动会话

完整源码的监听循环核心如下：

```cpp
for (;;)
{
    auto socket = co_await acceptor.async_accept(asio::use_awaitable);
    asio::co_spawn(io, echo_session(std::move(socket)), report_error);
}
```

- `async_accept` 等待新连接，成功后交付一个 socket。
- `std::move(socket)` 允许将资源移动给会话，避免复制；不是启动线程的语法。
- 这里用 `co_spawn`，是为了让监听循环继续接收其他连接。
- 如果改成 `co_await echo_session(...)`，监听协程就会等当前会话结束后才继续接收，业务结构已经不同。

可以把任务组织画成：

```text
普通 main
  ├─ 建立 io_context、退出信号处理
  ├─ co_spawn：启动监听协程
  └─ io.run：驱动所有事件
       └─ 监听协程：accept → 启动会话 → 再 accept
            ├─ 会话 A：读 → 写 → 再读
            └─ 会话 B：读 → 写 → 再读
```

### 5.4 你以后写新功能时，按这个顺序做

1. **选运行库及返回类型**：例如全程使用 Asio 的 `awaitable<T>`。
2. **划分任务**：一条连接一个会话；独立长期任务用 `co_spawn`，有先后依赖的子操作用 `co_await`。
3. **寻找真正的异步 API**：读、写、定时器都应由库支持；现成阻塞数据库调用不会自动异步化。
4. **明确结果和错误**：结果从 `co_await` 表达式取得；定义正常 EOF、超时、失败如何处理。
5. **明确资源寿命**：socket、buffer、任务参数是否能活过等待点？
6. **驱动运行环境**：提交任务后必须有相应事件循环运行。
7. **先测一个客户端，再测多个客户端与断开连接**；最后考虑超时、限流和关闭流程。

当前例子的 Ctrl+C / SIGTERM 会调用 `io.stop()`，是演示停止方式，不是“排空全部会话并发完响应”的生产优雅关停。

**本节完成标志：** 能指出 accept、read、write 各自的等待点，并说明为什么等待一个客户端时仍可服务另一个客户端。

## 第 6 步：另一种用法——用 co_yield 逐项生成数据

不是所有协程都为了网络。生成器可以在调用方每次需要时，计算下一个值。

### 6.1 运行配套生成器

打开 [03_generator_cpp20.cpp](../code/base_coroutines/03_generator_cpp20.cpp)，执行：

```bash
./generator
```

输出 1～5。先看业务函数和消费方式：

```cpp
IntGenerator numbers(int count)
{
    for (int n = 1; n <= count; ++n)
        co_yield n;
    co_return;
}

// 放在普通 main 中消费：
auto sequence = numbers(5);
while (sequence.next())
    std::cout << sequence.value() << '\n';
```

这里 `IntGenerator` 是同一源码里定义的教学类，不是 C++20 标准库类型。

### 6.2 一次 next 对应一次推进

| 操作 | 发生什么 |
|---|---|
| `numbers(5)` | 创建生成器，本例先不执行函数体 |
| 第一次 `next()` | 执行到 `co_yield 1`，保存 1 并暂停，返回 true |
| `value()` | 读取刚才产出的 1，不会推进任务 |
| 第二次 `next()` | 继续循环，产出 2，再暂停 |
| 产出 5 后再 `next()` | 函数完成，返回 false |
| 对象析构 | 回收协程帧；提前停止消费也要回收 |

仅在 `next()` 成功后读取 `value()`。本例会检查非法读取，而不是给你一个有效的“最后结果”。

生成器没有后台线程或事件循环。若生成值之前调用了阻塞 `read`，调用 `next()` 的线程仍会被阻塞。`co_yield` 不会自动把磁盘访问变成异步。

`std::generator` 是 C++23 才加入的库设施，且仍需实现它的标准库；本教程 C++20 示例没有依赖它。

**本节练习：** 把 `numbers(5)` 改成 `numbers(3)`；再在消费两项后 `break`。说明“函数没走到末尾”和“对象仍应正常回收”为什么可以同时成立。

---

## 第 7 步：现在再理解底层——谁保存，谁等待，谁恢复

到这里你已经会使用协程。以下内容帮助你看懂实现、排查问题，不要求你立刻实现生产协程库。

### 7.1 四个角色，先不要混在一起

| 角色 | 本仓库例子 | 职责 |
|---|---|---|
| 返回对象 / 所有者 | `Task` | 让调用者持有任务；本例负责最终回收帧 |
| promise 对象 | `Task::promise_type` | 与编译器约定如何创建返回对象、开始、结束、交付结果或记录异常 |
| awaiter（等待器） | `suspend_always`、epoll 示例的 Waiter | 说明一个等待操作是否就绪、如何登记、恢复后交付什么 |
| 调度者 | `main`、EventLoop、Asio | 决定何时执行哪个任务，调用或安排恢复 |

`promise_type` 不是 `std::promise`，不会自动产生 future 或线程。`Task` 是自定义名字，不是语言关键字。

### 7.2 协程帧与 handle

C++20 使用无栈（stackless）协程模型。需要跨暂停点保存的状态组织在**协程帧**中，不要求每个协程保留完整独立调用栈：

```text
Task 所有者 → coroutine_handle → 协程帧
                                  ├─ promise
                                  ├─ 按值参数
                                  ├─ 跨暂停点仍需要的局部状态
                                  ├─ 需要存活的 awaiter
                                  └─ 执行位置及编译器管理信息
```

帧常通过动态分配获得，符合条件时可由编译器消除分配。不是所有局部变量都必须固定存进帧，具体布局由实现和优化决定。

`std::coroutine_handle` 是访问这份状态的轻量句柄：

- `resume()`：恢复一个允许恢复的挂起协程，通常在调用它的线程上执行到下一次挂起或完成。
- `done()`：查询非空有效句柄对应的协程是否处于最终挂起。
- `destroy()`：销毁挂起协程的状态，清理仍存活的对象。

复制 handle 不是复制任务，也不增加引用计数；一个句柄销毁帧后，其他副本不会自动置空。handle 本身析构也不会自动回收帧，所以本例需要 Task 封装所有权。

暂停后，当前恢复调用可以返回，线程继续执行调用方代码。以后恢复时，再读取帧中的状态。CPU 没有“等待 C++ 协程网络”的特殊通用指令；编译器把这些操作转换成普通代码，库再把它们接到事件机制上。

无栈协程也不能让任意深层普通函数直接暂停外层调用。需要挂起的调用链，应使用可组合的协程返回类型和等待协议；普通同步 helper 仍按普通函数规则运行。

### 7.3 对照 task.hpp 认识 promise 协议

打开 [task.hpp](../code/base_coroutines/task.hpp)，先只找这些函数：

| 成员 | 本例做什么 |
|---|---|
| `get_return_object()` | 从 promise 取得 handle，构造调用方拿到的 Task |
| `initial_suspend()` | 返回 `suspend_always`，所以任务先创建、以后才执行函数体 |
| `return_void()` | 接收 `co_return;` 的无值结束 |
| `unhandled_exception()` | 保存函数体未处理的异常 |
| `final_suspend()` | 返回 `suspend_always`，完成后保留帧给 Task 回收 |

有值返回的类型使用 `return_value(value)`；同一个 promise 不能同时声明 `return_void` 和 `return_value`。生成器额外通过 `yield_value(value)` 定义 `co_yield` 的产出和等待行为。

Task 还用到了几个普通 C++ 写法：

- `using Handle = ...`：类型别名。
- `= delete`：禁止复制，避免两个对象同时负责销毁同一帧。
- `Task(Task&& ...)`：移动构造，转移所有权。
- `std::exchange(other.handle_, {})`：取走旧句柄，并把来源置空。
- `noexcept`：不向外传播异常的承诺。

本例的正常生命周期是：

```text
调用协程 → 建立帧和 promise → 创建返回 Task → initial_suspend
  → 调用者拿到 Task
  → 首次 resume → 执行函数体 → 中途 co_await 暂停
  → 再次 resume → 继续函数体 → co_return → return_void
  → 结束函数体局部对象的生命周期 → final_suspend
  → done() 为 true，但帧仍存在
  → Task 析构 → destroy → 清理 promise、参数存储等，回收帧
```

因此**任务完成不等于帧已经销毁**。这里最终挂起后不能再恢复，但还要回收帧。如果设计成最终不挂起而自动销毁，就不能照搬本例继续持有并销毁句柄的策略。

### 7.4 co_await 的结果从哪里来

**awaitable** 是能放在 `co_await` 后面的对象；**awaiter** 是最后真正提供等待协议的对象。二者有时是同一个对象，有时通过 `operator co_await` 或 promise 的 `await_transform` 转换。

先看最基本的协议形式（接口示意，不是独立可编译程序）：

```cpp
bool await_ready();
void await_suspend(std::coroutine_handle<> current);
Result await_resume();
```

对于 `auto result = co_await operation;`，顺序是：

```text
取得 awaiter → await_ready()
                ├─ true → await_resume() → 用返回值初始化 result，继续执行
                └─ false → 当前协程挂起 → await_suspend(current)
                              → 登记事件和恢复方式，交回执行权
                              → 事件发生，调度者恢复协程
                              → await_resume() → 初始化 result，继续执行
```

关键区分：

- `await_ready()` 的 `false` 不是整个 `co_await` 表达式返回了 false，只是“现在不能直接继续”。
- `await_suspend()` 安排等待，不一定创建线程。
- `await_resume()` 提供结果或抛出操作错误，**它不是负责调用 handle.resume 的调度者**。
- awaiter 可以不交付结果，如本例 readable 等待只表示“可以重新尝试读”。

`await_suspend` 也可返回 `bool`：true 保持挂起，false 继续执行；或返回另一个协程句柄，将执行权转给它。这些用于更高级的组合设计。

调用 `await_suspend` 时协程已经挂起。多线程库中，发布 handle 后可能已有另一线程恢复甚至销毁 awaiter，因此不能继续假定其内部状态安全可访问。初学时先使用库提供的等待对象，不要照着三成员接口就随意写多线程运行时。

### 7.5 把等待接到 Linux epoll

打开 [04_epoll_coroutines.cpp](../code/base_coroutines/04_epoll_coroutines.cpp)，运行：

```bash
./epoll_coroutines
./epoll_coroutines --cancel-demo
```

这一步不依赖 Boost；使用 Linux 的 socketpair、timerfd 和 epoll，在同一个线程调度四个任务，无需外部网络或监听端口。

- 两个 sender 分别等待 80 ms、30 ms 的 timerfd，然后发送 `hello A/B` 并关闭写方向。
- 两个 receiver 循环非阻塞 `recv`，没数据时暂停，遇到 EOF 后输出积累的数据。
- B 通常先完成，但精确耗时与次序不保证。

实际等待链路：

```text
receiver：recv → 返回 EAGAIN（暂时无数据）
  → co_await loop.readable(fd)
  → await_suspend：保存 {token, fd, handle}，调用 epoll_ctl 登记
  → receiver 挂起，线程回到事件循环
  → epoll_wait 等到 fd 就绪
  → loop 按 token 找到等待，先注销，再 handle.resume()
  → await_resume 返回
  → receiver 再次 recv，检查真实结果
```

这叫 **Reactor / readiness + 协程**。epoll 只报告就绪，应用仍执行真正的 `recv` / `send`；不是内核替你完成整个业务读取。

就绪不等于“完整消息已到达”，再次 `recv` 可能得到数据、EOF、错误，甚至再次 EAGAIN。示例使用水平触发注册，因此 EAGAIN 后、登记前刚好到达的数据也可以被观察到。

### 7.6 本例如何避免恢复已销毁的任务

EventLoop 保存的是**借用 handle**，`std::vector<Task>` 中的 Task 才是帧所有者。等待器在帧中存活，其析构会撤销注册。

```text
销毁等待中的 Task
  → destroy 协程帧
  → 帧内仍存活的 Waiter 析构，注销等待
  → 以后事件循环不能再恢复这个已失效 handle
```

`--cancel-demo` 预期输出：

```text
before task destruction: pending=1
after task destruction: pending=0
```

loop 使用递增 token，防止 fd 编号复用或旧事件造成错误匹配；本批次已取消的 token 会被忽略。

这个教学运行时有意保持简单：

- 只支持单线程登记与恢复，一个 fd 同时只有一个等待者。
- Task 启动后由 loop 接管恢复，调用方不能擅自再次恢复 I/O 等待中的任务。
- loop 必须比所有等待它的 Task 活得更久。
- 每次等待 ADD / DEL，不代表高性能生产注册策略。
- 没有通用 `Task<T>`、嵌套组合、跨线程恢复或完整取消传播。
- 5 秒无事件就报错，是演示防挂死，不是完整请求超时策略。

理解这一步，是为了能追踪等待链路，而不是要求你把这个小运行时直接用于生产。

## 第 8 步：开发时必须检查的安全边界

### 8.1 跨等待点的东西必须活得足够久

```cpp
Task work(std::string text);             // 按值，帧拥有自己的字符串
Task work_ref(const std::string &text);  // 仅引用，外部对象必须继续存活
```

协程暂停不会延长外部对象寿命。`work_ref(std::string("temporary"))` 的临时字符串可能在恢复前已经销毁；`string_view`、`span`、裸指针也不拥有数据。成员协程还要考虑 `this` 是否有效。

尽量用具名协程函数，把需要拥有的数据按值传入，或明确约定共享所有权 / 外部作用域。立即调用的带捕获临时 coroutine lambda 有额外风险：闭包很快销毁，恢复时可能访问失效捕获；**按值捕获不等于数据自动按值进入协程帧**。

### 8.2 不要跨等待点持有普通 mutex

```cpp
// 反例片段：不要这样组织异步业务。
std::lock_guard<std::mutex> lock(mutex);
co_await async_operation();
```

这里 `async_operation` 是示意名，需要具体库提供。协程暂停仍持锁时，同线程的另一任务若阻塞在这把锁上，会连带堵住事件循环，原任务也无法恢复。跨线程恢复还可能导致由错误线程解锁。

更好的方向是缩小同步锁作用域，在挂起前释放；或按运行库约定使用消息传递、strand、异步同步原语。

单线程也不等于没有逻辑竞态：A 读状态后暂停，B 修改状态，A 恢复时旧假设可能已失效。

### 8.3 异常一定要有人接收

协程体可能在创建调用之后很久才执行，异常不一定直接抛回创建任务的那一行。

- 教学 Task 在 promise 中存异常，由 `resume()` 或 EventLoop 检查重抛。
- Asio 中可以在协程内 `try/catch`，或让异常通过 `co_await` 传播给父任务。
- 独立启动的根任务要配置完成处理器；本教程用 `on_complete` / `report_error` 观察错误。

不要只启动任务，却没有任何错误观察路径。

### 8.4 取消、销毁、完成是不同动作

| 动作 | 含义 |
|---|---|
| 完成 | 任务正常结束，或异常结束并按协议记录 / 传播 |
| 取消请求 | 请求任务或异步操作停止，何时响应由运行库约定 |
| 销毁帧 | 清理挂起任务仍存活的对象，释放状态 |

取消不等于回滚已发送数据或已提交数据库事务。销毁暂停任务也不会自动运行它后半段的正常业务逻辑。必须协调 fd 等待、定时器、完成回调与 handle 寿命，不能留下迟到回调访问已释放帧。

不能随意对正在执行的协程调用 `destroy()`，也不能同时从多个线程恢复同一个协程。

### 8.5 明确线程模型，识别阻塞操作

单个 `io.run()` 调用线程可承载多个并发会话。若多个线程同时运行同一个 `io_context`，任务恢复可能落到不同线程；共享对象仍需相应 executor / strand / 同步策略。strand 保证关联处理不同时执行，不保证永久绑定一个 OS 线程。

`thread_local` 属于线程，不属于协程。不要把请求身份默认存在 TLS 里并假定恢复时永远不变。

协程中仍然会阻塞线程的常见操作包括：

- 阻塞 socket 上的 `recv`。
- `std::this_thread::sleep_for`。
- 同步数据库查询、同步磁盘 I/O。
- 长时间不挂起的 CPU 密集循环。

应换成真正异步 API，或投递到工作线程池再异步等待结果；后者也需要库支持，不能只给原函数加一个 `co_await`。

### 8.6 性能与资源不是免费的

协程适合等待多、业务分多步的任务，能改善异步控制流的可读性；它不自动增加 CPU 核心，也不保证比手工状态机快。

仍要考虑帧分配、每连接缓冲区、调度队列、I/O 注册、超时和背压。生产服务至少观察吞吐、P99 延迟、峰值内存、线程数、队列长度及超时 / 取消率。限制连接数和积压数据，避免“任务很轻就无限创建”。

## 排错速查：先看你的返回类型和运行环境

| 现象 / 报错 | 优先检查 |
|---|---|
| 找不到 `<coroutine>` 或关键字不识别 | 是否用 C++ 编译器、`-std=c++20`，以及支持它的标准库 |
| 普通 `void f()` / `int f()` 写 co_await 后找不到 promise type | 返回类型不适合协程；使用本文 Task 或所选库的 awaitable |
| 教学 Task 无法被 co_await | 此类型仅拥有根协程，没有实现可等待接口 |
| promise 同时含 return_void 和 return_value | 无值与有值协议必须二选一 |
| `co_await std::future` 失败 | 标准 future 没有内置这个等待接口，需框架适配 |
| `co_await sleep_for(...)` 失败 | 这是返回 void 的阻塞函数，不是可等待异步操作 |
| C++20 找不到 std::generator | 它是 C++23 库设施；本文用自定义 IntGenerator |
| Asio 头文件找不到 / 协程接口不匹配 | 检查 Boost 安装、实际版本和工具链；默认示例无需 Boost |
| 创建任务后没有输出 | 教学 Task 需要显式恢复；Asio 需要启动任务并驱动 io.run |
| 某个任务卡住，其他任务也不推进 | 查找事件线程中的阻塞调用、长循环、同步锁 |
| 恢复后崩溃或数据异常 | 先查引用、buffer、this、捕获和 handle 是否悬空 |

`main`、构造函数、析构函数不能直接定义为这类协程。保留普通 `main`，在里面创建 / 启动协程并驱动运行环境。

## 最后：按这份检查表独立完成一次开发

不要以“我看过所有术语”为完成标准。按顺序做到以下几点：

1. **不改 Task 实现**，从空文件写出第 1 步程序，解释两次恢复的输出。
2. 给任务增加第三阶段，正确安排恢复；再交替运行两个任务。
3. 用 C / C++11 状态机复现相同输出，指出进度和数据存在哪里。
4. 用 Asio 定时器启动多个任务，再通过 `co_await` 子协程取得一个结果。
5. 跑通 TCP 回显，解释 socket、buffer、EOF 和完成处理器的职责。
6. 跟踪 epoll 示例的 `EAGAIN → 登记 handle → 等事件 → resume → 再 recv`。
7. 给自己的新功能列出：**谁拥有任务，谁拥有数据，谁负责恢复，哪里处理错误，如何停止。**

到这一步，你已经具备开始写协程业务代码的基础；通用运行时、多线程调度与复杂取消机制可以在后续专题中继续学习。

进一步资料：

- [配套示例与详细运行说明](../code/base_coroutines/readme.md)。
- [I/O 复用](<base_IO multiplexing.md>)、[服务器框架](base_server_application_framework.md)：网络与事件循环基础。
- [Boost.Asio C++20 协程](https://www.boost.org/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html)。
- [Boost.Context 上下文概述](https://www.boost.org/doc/libs/latest/libs/context/doc/html/context/overview.html)。
- [C++20 草案：协程函数](https://timsong-cpp.github.io/cppwp/n4861/dcl.fct.def.coroutine)、[await 表达式](https://timsong-cpp.github.io/cppwp/n4861/expr.await)、[coroutine_handle](https://timsong-cpp.github.io/cppwp/n4861/coroutine.handle)。
- [Linux epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)。

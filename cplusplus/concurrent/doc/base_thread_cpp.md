# 现代 C++ 多线程开发：从会用到会组织

本篇只聚焦 C++ 标准库，不按 pthread API 逐个翻译。重点是：**谁执行任务、谁拥有数据、共享状态如何保护、任务怎样返回结果、线程怎样退出。** 配套小程序在 [base_thread_cpp](../code/base_thread_cpp/readme.md)，每个文件只展示一个主要问题。

第一次读时，先记住几个贯穿全文的词：

| 术语 | 在本篇中的意思 |
|---|---|
| 并发 | 多个任务的执行时间可以交叠；单核也能轮流执行 |
| 并行 | 多个任务在同一时刻实际执行，通常需要多个 CPU 核心 |
| 共享数据 | 两个或更多线程会访问的同一对象；尤其注意其中有线程修改它 |
| 同步 | 用语言/库规定的规则安排线程之间的数据访问，例如 mutex、join |
| 阻塞等待 | 当前线程暂时不能继续，等待条件满足；其他线程仍可运行 |
| 忙等/自旋 | 反复检查条件，等待期间仍消耗 CPU 执行机会 |
| 生命周期 | 对象从构造到销毁的期间；线程使用的对象必须仍然存活 |

**同步不是给线程“加一点延迟”。** `sleep_for()` 只让调用线程等待一段时间，不能保证另一线程已完成，也不建立共享数据访问的同步关系。

## 1. 实际开发先选结构，再选 API

多线程不只是 `std::thread(function)`。开始之前先明确：

| 问题 | 常见选择 |
|---|---|
| 有一个需要长期运行、可停止的后台执行流 | C++20 `std::jthread` + 明确的等待出口 |
| 只想执行一个任务并取得结果/异常 | `future`、任务执行器；简单实验可用 `std::async` |
| 多个线程维护一组共享数据 | mutex + RAII 锁 |
| 一个独立整数的原子更新 | atomic |
| 等待队列非空、状态就绪或关闭 | condition_variable + 谓词 |
| 大量任务需要控制并发量 | 有限工作线程 + 有界任务队列 |
| 大量网络连接等待 I/O | event loop 管连接，线程池处理适合的业务工作 |

新 C++20 代码中，能明确拥有 worker 生命周期时通常优先考虑 jthread。既有 C++11～17 工程仍广泛使用 thread、RAII join、mutex 和 condition_variable。标准 C++20 没有通用线程池或网络 event loop，需要应用或库提供。

| 标准 | 与本篇相关的能力 |
|---|---|
| C++11 | thread、mutex、lock_guard、unique_lock、condition_variable、atomic、future |
| C++14 | shared_timed_mutex、shared_lock |
| C++17 | shared_mutex、scoped_lock、锁类型模板参数推导 |
| C++20 | jthread、stop_token、semaphore、latch/barrier、atomic wait/notify |

配套中的线程参数/future 示例按 C++11 构建，其余为了使用 jthread 按 C++20 构建。shared_mutex/scoped_lock 本身从 C++17 就可用。

## 2. 第一个线程：创建、参数、join

### 2.1 先分清：线程、线程对象、主线程

**线程是执行代码的执行流；`std::thread` 是 C++ 中管理它的对象。** 两者不是同一个东西。

- 主线程：开始执行 `main()` 的线程。
- 工作线程：程序另外创建的线程，例如负责计算、处理任务。
- 线程对象：例如下面的 `worker`，记录它关联哪个线程，让你可以等待或分离这个线程。

同一进程里的线程共享地址空间，所以都能访问同一份共享数据；每个线程有自己的调用栈。操作系统负责调度线程，不保证“先创建的先运行”。单核上可以轮流执行，多核上可以同时执行。

```cpp
std::thread worker([]
{
    // 工作线程执行这里。
});

// 主线程继续执行这里，与工作线程的先后顺序未确定。
worker.join();
// 主线程走到这里时，工作线程已经结束。
```

创建线程之后，不需要再调用一个 `start()`。新线程甚至可能在 `std::thread` 的构造函数返回之前开始执行。

### 2.2 join：调用者等待这个线程结束

`worker.join()` 的意思是：**当前调用它的线程，在这里等待 worker 关联的线程执行完毕。** 通常是主线程调用，但也可以由其他线程调用。

```text
主线程：创建 worker ── 做自己的工作 ── join 等待 ── 继续执行
工作线程：    开始执行 ─────────────── 结束
```

理解这几个细节：

- `join()` 不启动工作线程，工作线程在创建时就已经启动。
- `join()` 不要求工作线程退出，也不杀死它；工作线程不结束，调用者就可能一直等待。
- 等待的是调用者，进程里的其他线程仍可以运行。
- 工作线程已经结束时，`join()` 不需要再等它执行任务，但仍要完成关联资源的回收和同步。
- 成功返回后，这个线程对象不再关联该执行线程，不能对它再 `join()` 一次。

可以把它理解为“等这个任务干完，再处理后面的事”。调用 `join()` 之前，主线程和工作线程可能并发访问数据；它不能替代执行期间需要的锁。

[示例 01](../code/base_thread_cpp/01_thread_cpp11.cpp)：

```cpp
int result = 0;
std::thread worker(calculate, 21, std::ref(result));
worker.join();
std::cout << result;
```

构造 worker 就开始创建新执行线程；不是调用 join 才开始。新线程运行 calculate(21, result)，main 自己仍可以继续执行。

`join()` 等目标完成并建立同步，返回后 main 可以读取该线程之前写的结果。它不是“强制结束线程”，也不自动返回函数的 return value。

### 2.3 参数如何传递

thread 会为启动保存可调用对象和参数。普通参数通常按复制/移动保存；函数需要引用时，显式 `std::ref`。main 的 result 必须一直存活到 worker 不再使用，示例由 join 保证。

lambda 的 `[&]` 捕获引用、`[value]` 捕获值。例如创建一组任务时：

```cpp
// i 按值保存，其他外部变量引用；其他对象也必须足够长寿。
workers.emplace_back([&, i] { process(i); });
```

别用 `[&]` 让所有线程引用不断变化的循环变量 i。把指针传给线程同样不会延长对象寿命；成员函数捕获 this 也要确保所属对象尚未销毁。

### 2.4 joinable：这个对象是否仍关联一个线程

`worker.joinable()` 返回 `bool`。**它查询线程对象的关联状态，不查询线程任务是否正在运行。** 调用它不会等待，也不会改变状态。

| 线程对象的状态 | `joinable()` |
|---|---|
| `std::thread worker;`：只创建空对象，没有启动线程 | `false` |
| 用函数创建线程，尚未 join/detach | `true` |
| 线程函数已经执行完，但尚未 join/detach | **仍是 `true`** |
| 已成功调用 join | `false` |
| 已成功调用 detach | `false` |
| 关联已通过移动转交给其他对象 | 原对象为 `false`，新对象接管关联 |

线程函数执行完，只代表“任务结束”；对象仍有关联需要处理。因此这两件事可以同时成立：**线程已经不执行代码，`joinable()` 还是 true。**

```cpp
std::thread worker;                // 空对象：false。
worker = std::thread([] {});       // 建立关联：true。

if (worker.joinable())
{
    worker.join();                // 等待结束并解除关联。
}

// 此时 worker.joinable() 为 false。
```

这里即使空函数瞬间执行完，第一次 join 之前仍然是 joinable。不能靠 `sleep_for()`、打印输出或 `joinable()` 来判断任务是否完成；需要等待完成时用 join，需要观察业务进度时设计受同步保护的状态或使用 future。

`if (worker.joinable())` 常用于清理可能为空、被移动或已经处理过的线程对象。它不使多线程同时操作同一个 `worker` 对象变得安全；线程对象的生命周期也应有明确所有者。

### 2.5 detach：解除对象关联，让线程独立继续执行

`worker.detach()` 的意思是：**线程对象不再管理这个执行线程，也不等待它结束。** 线程继续执行；结束后由运行时/操作系统回收对应的线程资源。

```text
主线程：创建 worker ── detach ── 继续执行，不等 worker
工作线程：    开始执行 ─────────── 结束
```

成功 detach 后，原来的 `worker.joinable()` 为 false，不能再通过它 join、取得结果或判断该任务是否结束。detach 自身不提供“任务完成”的同步，也不会自动使线程访问的共享数据安全。

**detach 不等于“永久后台运行”。** 它只是让线程脱离这个 C++ 对象；线程仍属于原进程，`main()` 返回导致进程结束时，分离线程也无法继续执行。

尤其不能这样写：

```cpp
void start_bad_task()
{
    int value = 42;

    std::thread worker([&value]
    {
        // 错误示范：函数可能已经返回，value 已经销毁。
        std::cout << value << '\n';
    });

    worker.detach();
}
```

这个程序可能出现悬空引用，行为未定义。detach 不延长局部变量的寿命；把捕获改成按值只能解决这个变量的寿命问题，仍需要安排任务完成、异常处理和进程退出。

刚入门时，普通工作线程优先用 join 或 jthread 管理。只有真正设计好独立任务的资源寿命、完成通知和退出机制时，才考虑 detach。

### 2.6 三个接口一起记，以及忘记处理的后果

| 接口 | 作用 | 等待任务结束吗 | 成功后 joinable |
|---|---|---|---|
| `joinable()` | 查询对象是否关联线程 | 不等待 | 状态不变 |
| `join()` | 等待关联线程结束并解除关联 | 等待；已结束则无需等待任务 | `false` |
| `detach()` | 解除关联，线程独立继续 | 不等待 | `false` |

**`std::thread` 对象析构时，如果仍然 joinable，会调用 `std::terminate()`，通常使整个进程异常终止。** 即使线程函数已经执行完，也适用这一规则。

所以 `std::thread` 不能像普通局部整数一样“离开作用域就不用管”。必须在它析构之前成功 join、detach，或把关联移动给另一个负责清理的对象。普通 thread 的析构函数不会替你自动 join。

对不可 join 的对象调用 join/detach 会抛出 `std::system_error`；线程也不能 join 自己，否则形成自我等待。已经 join/detach 的对象可以重新接管一个新线程，但这不是重启原线程。

**正常路径写了 join，并不保证异常路径安全。** 如果创建 worker 后，主线程在 join 之前抛异常或提前返回，worker 析构时仍可能 joinable。C++11～17 通常用负责 join 的 RAII 所有者；C++20 可以用下一节的 jthread。

最后区分两个动作：**要求停止**是通知线程尽快结束自己的任务；**join**是等待它已经结束。它们经常配合使用，但各自负责不同的事。

## 3. C++20 的 jthread 与协作停止

[示例 02](../code/base_thread_cpp/02_jthread_stop_cpp20.cpp)使用：

```cpp
std::jthread worker([&](std::stop_token stop)
{
    std::unique_lock lock(mutex);
    changed.wait(lock, stop, [] { return false; });
    stopped = stop.stop_requested();
});
worker.request_stop();
worker.join();
```

jthread 能把 stop_token 传给合适的入口；析构时若 joinable，会请求停止再 join。但线程仍需要响应请求。这里采用支持 token 的 condition_variable_any::wait，即使停止请求先于 wait，也能被观察到。[jthread 定义](https://timsong-cpp.github.io/cppwp/n4861/thread.jthread.class)

普通 `std::condition_variable::wait` 不接收 stop_token；阻塞 recv、sleep_for、semaphore acquire 也不会因为 request_stop 自动取消。需要自己设计退出路径，例如关闭队列并通知、使用可取消等待，或通过 I/O runtime 发出取消。

jthread 解决的是所有权与协作停止接口，不保证析构立刻返回。worker 无限循环且不响应请求，析构仍可能永久等待。

这里涉及的三个名字分别是：

- `stop_token`：线程收到的“停止状态观察接口”，可调用 `stop_requested()` 检查是否有人请求停止。
- `request_stop()`：发出停止请求；不强制杀线程，不保证调用返回时线程已结束。
- `join()`：等待线程真正结束。通常先请求停止，再等待退出。

这种方式称为**协作停止**：发请求的一方与执行任务的一方合作。线程要在适当位置检查请求，或采用支持停止请求的等待接口。

### 普通有限任务怎样用 jthread

示例 03/04 的 worker 只做固定次数计算，可以直接完成，不必为了用 jthread 添加 stop_token。它们显式 join 是为了在输出结果前确定两个线程已结束。

## 4. 共享数据的默认工具：mutex

`mutex` 是互斥量：同一时刻，最多一个线程拥有这把锁。其他线程尝试获取它时可能需要等待。**临界区**是持有锁时执行的、访问共享数据的代码范围。

锁不会自动找到并保护某个变量；所有可能冲突的访问都必须遵守同一套加锁规则。一个线程加锁，另一个线程绕过锁直接修改数据，仍然不安全。

[示例 03](../code/base_thread_cpp/03_mutex_cpp20.cpp)让两个线程各增加 10000：

```cpp
{
    std::lock_guard lock(mutex);
    ++counter;
}
```

所有冲突访问采用同一个 mutex；离开作用域自动 unlock。main 在全部 join 后读取最终 counter，所以此时不用再加锁。

`std::lock_guard lock(mutex);` 使用了 C++17 的类型推导。C++11 写作 `std::lock_guard<std::mutex> lock(mutex);`，含义相同：构造 lock 时获取锁，销毁 lock 时释放锁。

这里的**作用域**就是一对 `{ ... }` 限定的代码范围。**RAII** 是“利用对象生命周期管理资源”的习惯：对象构造时取得资源，析构时释放资源。所以这个例子不需要手写 unlock；正常走完、return 或正常异常展开，都会销毁已经构造的局部 lock 对象。

无同步地并发修改普通 int 是 C++ 数据竞争，行为未定义，不只是“结果偶尔少一点”。`volatile`、sleep 或“这个整数很小”都不能替代同步。

### 三种常用锁对象

| 类型 | 主要用途 |
|---|---|
| lock_guard | 进入作用域持锁，退出解锁；最简单 |
| unique_lock | 能主动 unlock/relock、移动、延迟获取；配合条件变量 |
| scoped_lock | C++17，可同时获取一把或多把锁，作用域管理 |

不是变量名字叫 lock 就自动保护了数据。保护范围必须涵盖共享数据所需的完整不变量，例如“检查余额并扣款”应按同一协议进行。

### 锁的范围要明确

从共享队列取任务时持锁，取完后释放锁，再执行业务。不要把慢 I/O、数据库调用、外部 callback 或长计算无条件塞进锁内，否则别的线程也会等待这些工作。

异常展开能触发 lock_guard 解锁，但它不会回滚你已经改了一半的业务数据；必要时先计算新值，再在短临界区提交。

## 5. 原子操作与内存顺序：先从每个词讲起

### 5.1 普通整数为什么不能随便由两个线程递增

`++counter` 看起来只有一行，但逻辑上涉及“读取旧值、计算新值、写回”。假设 counter 最初为 0，两个线程可能都读到 0，各自计算 1，再各自写入 1，两个增量就只剩一个。

这是帮助理解的执行交错；**在 C++ 中，两个线程无同步地读写同一个普通 int，构成数据竞争，行为未定义。** 实际后果不限于少加几次，编译器也不必按上述步骤执行。

解决方法可以是上一节的 mutex，也可以在单个计数器场景使用 `std::atomic<int>`。

```cpp
std::atomic<int> counter{0};
counter.fetch_add(1);
```

逐个看：

- `std::`：这个名字属于 C++ 标准库的命名空间。
- `atomic<int>`：保存 int 的原子类型；需要 `#include <atomic>`。
- `{0}`：把初始值设为 0，此时还没有并发访问。
- `fetch_add(1)`：一次原子“读—改—写”，加 1，并返回增加之前的值。
- “原子”：其他线程的原子访问不能插入这个操作的内部步骤，从而破坏这一次更新。

如果原值为 7，`int old = counter.fetch_add(1);` 后 old 为 7，这次操作把 counter 更新为 8。之后其他线程仍可能继续增加它。

### 5.2 load、store、fetch_add 分别干什么

| 写法 | 含义 | 返回值 |
|---|---|---|
| `counter.load()` | 原子读取当前值 | 本次读到的值 |
| `counter.store(10)` | 原子写入 10 | 无 |
| `counter.fetch_add(1)` | 原子读取旧值并增加 1 | 修改前的值 |
| `counter.exchange(10)` | 原子读取旧值并替换为 10 | 修改前的值 |

**多次原子操作拼在一起，不自动成为一个整体的原子操作。** 例如下面两步之间，其他线程仍可修改 counter：

```cpp
int old = counter.load();
counter.store(old + 1);           // 不能替代 fetch_add(1)。
```

需要“仅当当前值等于预期时才更新”时，还有 `compare_exchange`，即比较并交换（CAS）。它适合更复杂的原子协议，这篇入门代码暂不需要。多个字段必须一起保持一致时，优先考虑 mutex。

### 5.3 memory_order 是原子操作的参数，不是另一把锁

看 [示例 04](../code/base_thread_cpp/04_atomic_cpp20.cpp)：

```cpp
counter.fetch_add(1, std::memory_order_relaxed);
```

第一个参数 1 指定“增加多少”；第二个参数指定**这次原子操作对其他内存访问提供什么顺序与同步保证**。

`std::memory_order_release` 同样是一个可传入的标准库常量，不是函数，不会自己运行，也不是“释放内存”或“解锁”的命令。C++11 就有这些内存顺序；C++20 也支持写成 `std::memory_order::release`。

这里需要分清两件事：

1. **原子性**：对这个 atomic 对象的一次操作不可分割。
2. **同步与可见性**：其他普通数据的访问，什么时候能安全地发生在这个操作之后。

所有这些内存顺序都保留原子操作本身的原子性；它们的区别在于第二件事。不能因为一个标志位是 atomic，就认为它旁边的普通变量都自动受到保护。

### 5.4 relaxed：只需要这个原子值自身的正确更新

`relaxed` 可以理解为“对其他内存访问不额外建立同步关系”。它仍保证原子性，同一个原子对象的修改仍有一致的修改顺序；不是“随意写入、可能丢增量”。

示例 04 中，两个线程各执行 10000 次原子 fetch_add，主线程 join 两个线程后才读取结果，所以最终为 20000：

```cpp
counter.fetch_add(1, std::memory_order_relaxed);
```

这里不通过 counter 告诉其他线程“某份普通数据已经准备好了”。最终读取之前，线程完成的同步由 join 提供，因此独立计数使用 relaxed 足够。

如果把 counter 当成“队列任务已写好”“某个对象已初始化”的标志，就不能直接照搬这个结论。

### 5.5 release / acquire：通过一个原子变量交接数据

最典型的问题是：写线程先准备普通数据，再通知读线程可以读取。

```cpp
// 共享变量：在线程启动之前创建。
int payload = 0;
std::atomic<bool> ready{false};
```

写线程执行：

```cpp
payload = 42;
ready.store(true, std::memory_order_release);
```

读线程执行：

```cpp
if (ready.load(std::memory_order_acquire))
{
    // 这个例子只有一次写入，发布后不再修改 payload。
    std::cout << payload << '\n';
}
```

理解为两步配合：

- **release，发布**：通过 ready 发布这个线程在它之前准备好的数据。
- **acquire，获取**：读取 ready；如果读到了对应 release 写入的值，之后就能安全读取被发布的数据。

不是任何 release 和任何 acquire 都能配对。这里它们操作的是**同一个 ready**，而且读线程确实读取到了写线程发布的 true。读到初始 false 时，不能据此读取 payload。[C++20 标准草案的原子同步规则](https://timsong-cpp.github.io/cppwp/n4861/atomics.order)

```text
写线程                              读线程
payload = 42
      │ 本线程内先执行
ready.store(true, release) ───────→ ready.load(acquire) 读到这个 true
                                           │ 之后才读取
                                      使用 payload，得到 42
```

上述箭头表示 C++ 内存模型中的同步关系，不是“操作系统暂停一个线程，让另一个先执行”。读线程可能先运行并读到 false；release/acquire 不负责等待线程。

**happens-before（先发生于）**是描述这种安全顺序的术语。这里“写 payload”先发生于“读 payload”，所以这两个普通内存访问不构成数据竞争。它是一种由语言规则保证的关系，不是看两条日志时间戳谁早谁晚。

如果把发布与读取都改成 relaxed，ready 自身仍能安全读写，但它不建立这条普通数据的同步关系；这个例子中 payload 的写和读会构成数据竞争。不能把错误仅理解为“偶尔读到旧值”。

发布之后也不能继续随意修改 payload；release/acquire 并没有锁住它。重复发布、覆盖缓冲区或多个生产者，需要另行设计协议。

### 5.6 为什么配套程序用 wait，而不是只 load 一次

上面的 if 只检查一次；如果当时还没准备好，就跳过读取。真正想“等到数据准备好”，可以使用 C++20 原子等待。[示例 10](../code/base_thread_cpp/10_atomic_publish_cpp20.cpp)：

```cpp
// 写线程：先准备数据，再发布，最后通知。
payload = 42;
ready.store(true, std::memory_order_release);
ready.notify_one();

// 读线程：值仍为 false 时等待，值改变后返回。
ready.wait(false, std::memory_order_acquire);
observed = payload;
```

`wait(false, ...)` 中的 false 是“旧值”：值仍等于 false 时继续等待；观察到不同值后返回。在这个 bool 只从 false 改为 true 的例子里，返回就意味着观察到了 true，acquire 负责获取发布的数据。

三个职责要分开：

- `store` 改变 ready 的状态，release 负责发布。
- `notify_one` 给等待者继续检查的机会；它不改变 ready，也不代替 release/acquire。
- `wait` 等待值变化，并按指定内存顺序读取；它不调用主线程的 join。

如果先 store/notify，读线程后执行 wait，读线程发现 ready 已是 true，可以直接返回。如果只 notify 而不修改 false，wait 不会因此完成。这个接口避免手写不停 load 的忙等循环，但不保证实现完全不进行短暂自旋。[C++20 原子等待说明](https://timsong-cpp.github.io/cppwp/n4861/atomics.wait)

这里只交接一次。若值快速从 false→true→false，等待者可能错过中间的 true；原子等待不是保存每次事件的消息队列。

### 5.7 其他内存顺序：知道用途，先不要到处替换

| 内存顺序 | 入门理解 | 常见操作 |
|---|---|---|
| `relaxed` | 保证这个原子操作，不能靠它发布其他数据 | 独立计数的 fetch_add |
| `release` | 发布本线程在它之前准备的数据 | store；某些读—改—写操作 |
| `acquire` | 读到匹配的发布后，获取那些数据 | load、wait；某些读—改—写操作 |
| `acq_rel` | 一次读—改—写同时具有获取和发布语义 | exchange、fetch_add 等 |
| `seq_cst` | 顺序一致性：有获取/发布能力，并给所有 seq_cst 操作一个共同总顺序 | 常用原子操作的默认参数 |
| `consume` | 基于数据依赖的较复杂规则 | 初学不使用，优先 acquire |

这些参数不能随意填：store 不能用 acquire/acq_rel；load 和 wait 不能用 release/acq_rel。纯读、纯写和读—改—写操作的职责不同。

`seq_cst` 不会把所有线程变成串行执行，也不保证你每次都读到“墙上时钟意义上的最新值”，更不会自动修复其他普通变量的数据竞争。它让原子操作的整体顺序更容易推理。

不写第二个参数时，常用原子操作默认使用 `std::memory_order_seq_cst`：

```cpp
ready.store(true);
bool done = ready.load();
```

对于前面的一次发布场景，使用默认的 store/load 也可以建立所需同步，前提仍是读取到了相应发布值。刚开始写代码可以先用默认顺序；确实需要优化并能说明同步关系时，再选择更弱的顺序。

### 5.8 底层为什么需要这些规则

编译器可以在不改变合法单线程行为的前提下优化代码；CPU 也可能有乱序执行、写缓冲和缓存。源代码中“先写 A，再写 B”，不能单靠这一点就推出另一个线程“看到 B 时一定能安全读 A”。

C++ 的内存模型规定程序可以依赖什么；编译器再为目标 CPU 生成满足规则的指令。某种处理器上 release/acquire 可能不需要单独的屏障指令，其他处理器上可能需要相应指令，但源码中仍要写对同步语义。

不要把 release 解释成“把所有缓存刷新到主内存”，也不要把 acquire 解释成“重新读取所有内存”。这里讨论的是语言保证的顺序和同步；缓存一致性和 CPU 指令只是实现的一部分。

atomic 不保证每一种类型都无锁，可以通过 `is_lock_free()` 查询相应对象的实现。即使无锁，多个核心争用一个原子计数器也可能很慢；原子操作不是自动的性能优化。

### 5.9 写代码时怎么选

- 一个普通共享数据结构，例如队列、账户或多个关联字段：先使用 mutex。
- 一个独立计数器：用 atomic；明确只做统计且有适当完成同步时，可以 relaxed。
- “先写数据，再交给另一线程”：需要真正的同步，使用 mutex、future 或设计好的 release/acquire 协议。
- 不确定内存顺序：先使用 atomic 的默认 seq_cst，仍要检查普通数据的访问是否安全。
- 需要等待条件：使用条件变量或 atomic wait，不用 sleep 猜执行顺序。

## 6. 条件变量：等待一个真实条件

[示例 05](../code/base_thread_cpp/05_condition_cpp20.cpp)的共享状态是 ready/value，通知只是辅助。

**条件变量**是让线程等待状态变化的工具，本身不保存你的业务数据。它通常和 mutex 一起用：锁负责安全检查/修改状态，条件变量负责在条件不满足时等待。

```cpp
std::unique_lock lock(mutex);
changed.wait(lock, [&] { return ready; });
result = value;
```

过程是：持锁检查 ready；false 时原子协调释放锁与等待；醒来后重新持锁，再检查 ready；true 后读取 value。

`[&] { return ready; }` 是一个 lambda，也就是临时的小函数；`[&]` 让它通过引用访问外面的 ready。这个返回 bool 的检查函数叫**谓词**：true 表示可以继续，false 表示还需要等待。

`unique_lock` 用在这里，是因为 wait 需要暂时释放锁、醒来后再获取锁；lock_guard 没有这种手动释放/重新获取的接口。wait 返回时仍持有锁，因此接着读 value 仍在临界区内。

**虚假唤醒**指没有满足业务条件，等待也可能醒来。另一个情况是通知确实发生了，但其他消费者先拿走了数据。因此“醒了”不等于“能干活了”，必须重新检查谓词。

谓词版本处理了虚假唤醒以及“醒来但条件被别人改变”。不要写一个没有状态检查的 wait，然后默认每次醒来都意味着数据可用。

### 为什么通知先发也能正确

main 持同一 mutex 设置 value/ready，再 notify。如果 reader 更晚才来，它发现 ready 已经 true，直接继续。**是 ready 保存了状态；条件变量本身不会储存过去的通知。**

通知可以在解锁之后发出；示例这样写，减少被唤醒线程马上又争用仍被持有的锁。notify_one 唤醒某个等待者，notify_all 用于全部等待者需要重新检查状态的场景，常见于关闭/停止。

有总期限时以 steady_clock 的截止时刻配合 wait_until；不要每次虚假唤醒后重新开始一整段 timeout。

## 7. 获取结果与异常：future

[示例 06](../code/base_thread_cpp/06_future_cpp11.cpp)：

```cpp
auto result = std::async(std::launch::async, []
{
    return 21 * 2;
});
std::cout << result.get();
```

future 代表以后可取得的结果，get 需要时等待并取值。如果任务内部抛异常，异步共享状态保存它，get 在调用者重抛。

**异常**是 C++ 用 `throw` 报告失败、用 `try/catch` 处理失败的机制。future 可以把任务的异常带回调用 get 的线程；普通线程的 join 只负责等待，不能这样传递异常。

std::thread/std::jthread 入口的 return value 被忽略，join 不提供这种结果/异常通道。异常若直接逃出线程入口，会 terminate；应在入口捕获，再交给 promise、exception_ptr 或任务系统。

### async、promise、packaged_task 的区别

| 类型 | 职责 |
|---|---|
| future | 消费一个异步结果，通常 get 一次 |
| promise | 生产结果，可手动 set_value/set_exception |
| packaged_task | 包装函数，执行后把结果/异常放入共享状态 |
| async | 为函数启动/延迟执行任务，返回 future |

这里显式使用 `launch::async`；默认 async 策略还可能采用 deferred，在等待者调用 get/wait 时执行。不要忽略策略却假设肯定有新线程。[async 规则](https://timsong-cpp.github.io/cppwp/n4861/futures.async)

async 不是标准线程池接口。高频大量任务不宜机械地每项都启动 async；async 返回的 future 在特定条件下析构也可能等待任务，因此要安排其生命周期。future 不自带通用取消。

## 8. 读多写少：shared_mutex

[示例 07](../code/base_thread_cpp/07_shared_lock_cpp20.cpp)：writer 采用 unique_lock，reader 采用 shared_lock。

```cpp
std::shared_lock lock(mutex);
snapshot = value;
```

共享模式允许多个读者同时持锁，写者需要独占。共享锁下不能任意修改受保护的数据；普通 shared_ptr 的引用计数安全也不意味着 pointee 自动受保护。

C++14 已有 shared_timed_mutex/shared_lock；C++17 加入 shared_mutex。示例用 jthread 管理 writer，所以整体按 C++20 构建。

reader 的 snapshot 是当时观察值，不保证等于最后1000；main 在 join 后读取 final，才可确认写者全部完成。读写锁可能有额外竞争与饥饿，读多也不一定比 mutex 更快。

## 9. 两把锁：避免相反获取顺序

[示例 08](../code/base_thread_cpp/08_scoped_lock_cpp20.cpp)让两个线程执行 A→B 和 B→A 转账：

```cpp
std::scoped_lock lock(from.mutex, to.mutex);
--from.balance;
++to.balance;
```

scoped_lock 用多锁获取协议，避免手工“持有第一把，再等待第二把”的环路。两个实参必须是不同 mutex。本例完成后余额都回到1000。

C++11 可用 `std::lock` 和 defer_lock 的 unique_lock 实现类似方式；C++17 的 scoped_lock 简化作用域管理。但它不能解决“持锁等待网络/条件/其他资源”形成的所有死锁。

## 10. 常见后台 worker：队列加关闭状态

[示例 09](../code/base_thread_cpp/09_worker_queue_cpp20.cpp)只有一个 worker 和五个任务，便于观察实际流程：

```text
main：提交 job → notify
worker：等待“有 job 或已关闭” → 取出 job → 解锁 → 处理
main：设置 closed → notify_all → join
worker：把剩余 job 排空，空且关闭时退出
```

`jobs` 与 `closed` 由同一 mutex 保护。worker 等待的谓词是 `closed || !jobs.empty()`，既处理工作，也给等待状态一个退出条件。

正常结束用 closed 排空，而不提前 request_stop。停止 token 则给异常退出/外部停止提供另一条等待出口。当前代码保留明确区分：**正常关闭排空，停止请求允许在无工作时退出。**

生产服务需要决定取消时是否丢弃/排空任务、怎样通知结果、任务自身能否取消，不要含糊地把所有 stop 都当作“已经完成”。

这个小例子只投递五个整数，没有通用线程池和队列容量控制。实际服务应采用有界队列和拒绝/背压策略；可继续阅读已有 [BoundedQueue](../code/base_multi_thread/bounded_queue.hpp)，无需先写几百行线程池才能理解线程组织。

### 成员和对象的销毁顺序

mutex、condition_variable、队列等共享对象必须比 worker 活得久。局部变量后构造先销毁，示例先创建共享对象，再创建 jthread。作为类成员也要安排顺序，并在销毁共享成员前明确停止/join。

不能拿着 worker 退出时需要的 mutex 去 join，否则 main 等 worker，worker 等 main 释放锁，双方都无法继续。

## 11. 其他 C++20 工具何时用

| 工具 | 解决的问题 | 与 mutex/条件变量的区别 |
|---|---|---|
| counting_semaphore | 控制最多多少任务占资源 | 计数许可；不同线程可获取/释放 |
| binary_semaphore | 简单许可交接 | 不是带拥有者检查的 mutex |
| latch | 一次性等 N 项完成 | 倒计数到0后放行，不能重置成新轮 |
| barrier | 多线程按阶段汇合 | 可重复阶段，用于分阶段并行计算 |
| atomic wait/notify | 等一个原子状态变化 | 等值变化，不替代复杂业务谓词 |

对应完整小实验已有 [modern_cpp20.cpp](../code/base_multi_thread/modern_cpp20.cpp)。本目录先把主要开发路径做短，不重复堆叠工具展示。

## 12. 内核、性能与 I/O 的实际边界

Linux 上常见 C++ 标准库借 pthread/NPTL 实现线程，一般对应内核可调度线程。标准本身不强制所有平台采用相同实现。线程拥有执行栈和 TLS，同进程共享地址空间、fd 等资源。

mutex 无竞争时通常走用户态原子快路径，竞争时可能自旋或用 futex 睡眠；加锁不等于每次都做系统调用。CPU cache 和内存顺序由语言同步协议约束，不需要手工“刷内存”。

线程数也不是越多越好：CPU 计算常从有限 worker 开始，阻塞 I/O 池需要按等待比例与资源预算设置。hardware_concurrency 只是提示，可能返回0，不应直接当固定线程数量公式。

大量连接更适合 event loop 管理 I/O，工作池处理业务；不要让每个连接都占一个线程，也不要在 I/O 线程里无期限等待业务队列空间。更重要的是单连接/任务的所有权、上限、超时和错误恢复。

性能测量看吞吐、延迟、队列等待、锁竞争、线程数和内存；先确保正确，再减少争用和复制。不要为了“现代”直接把明确的 mutex 协议改成复杂无锁算法。

## 13. 实际编码时检查这几项

1. 参数/捕获引用是否活到 worker 完成？
2. 每个共享可写对象是线程独占、mutex 保护，还是明确的 atomic 协议？
3. 所有阻塞等待都有结束条件吗？
4. 线程内异常由谁捕获、谁观察？
5. 正常关闭是排空还是丢弃，停止请求能不能唤醒等待？
6. join 是否发生在共享对象销毁之前，且不持有它需要的锁？

配套代码无需第三方库，运行顺序和预期输出见 [README](../code/base_thread_cpp/readme.md)。想对照 pthread/书中章节，可以回看 [base_multi_thread.md](base_multi_thread.md)。

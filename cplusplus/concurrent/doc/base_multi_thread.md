# Linux 多线程编程：POSIX pthread 与 C++11～C++20

本文按游双《Linux 高性能服务器编程》第二篇第 14 章 **14.1～14.8** 编排，面向刚开始学习多线程的读者。先补共享内存、内核调度和同步基础，再对照书中的 pthread API 与现代 C++ 标准库写法。

**原文依据**：已读取你提供 PDF 的第 14 章正文，PDF 阅读器页码 **505～540**。下文“书中对照”以自己的话归纳，随后说明现代 Linux/C++ 的实践与部分需要修正的旧表述。配套示例为新写程序：[base_multi_thread](../code/base_multi_thread/readme.md)。

你的 C11-C20 在本篇分成两条标准线：C11/C17 用于展示 POSIX 原始 API；C++11/14/17/20 用于对照 RAII、标准线程、锁和同步设施。pthread 是 OS/POSIX 接口，不是 ISO C/C++ 标准库成员。

## 章节与学习路线

| 原书章节 | PDF 页码 | 学习重点 | 示例 |
|---|---:|---|---|
| 14.1 | 505～510 | 线程模型、LinuxThreads 与 NPTL | 第 0/14.1 节原理 |
| 14.2 | 511～514 | create/exit/join/cancel | `pthread_basics lifecycle/cancel` |
| 14.3 | 515～518 | detach、栈、guard、调度属性 | `pthread_basics lifecycle` |
| 14.4 | 519～520 | POSIX semaphore | `pthread_basics semaphore`、C++20 semaphore |
| 14.5 | 521～527 | mutex、属性、死锁 | POSIX/C++ counter、scoped_lock |
| 14.6 | 528～530 | cond wait/signal/broadcast | `condition`、有界队列 |
| 14.7 | 531～533 | 同步机制封装类 | RAII、BoundedQueue、ThreadGroup |
| 14.8 | 534～540 | 可重入、fork、信号 | `pthread_environment` |

建议先阅读第 0 节，再运行 lifecycle 和两个 counter，接着学习 semaphore、condition/queue、死锁预防，最后看取消、fork 和信号。这些是循序渐进的小实验，不是完整线程池服务器。

## 0. 从进程、内存和 I/O 开始

### 0.1 线程共享什么

一个进程有自己的地址空间与资源。进程内多个线程可以同时执行函数，并访问共享堆、全局变量和 fd。每个线程有自己的栈、寄存器执行现场、线程局部存储（TLS）、errno 和信号掩码。

```text
进程
├─ 共享：代码、全局数据、堆、fd 表、信号处理设置
├─ 主线程：栈/寄存器/TLS
├─ 工作线程 A：栈/寄存器/TLS
└─ 工作线程 B：栈/寄存器/TLS
```

“每线程有自己的栈”不等于其他线程无法访问栈上的对象：把某局部变量地址交给另一个线程后，它也可能访问该变量。因此这个对象必须活得足够久，且并发访问必须同步。

线程可能在不同 CPU 核心同时执行，也可能被调度器交替执行。**并发**是任务交错推进，**并行**是同一时刻多核执行。睡眠、阻塞 I/O、join 和锁等待都可能让当前线程暂时放弃 CPU。

### 0.2 用户态/内核态和上下文切换

应用代码与 pthread/标准库的大部分普通操作运行在用户态。创建线程、某些等待/唤醒或 I/O 请求需要内核支持。内核调度不同线程时，保存和恢复必要寄存器上下文，也可能影响缓存。

系统调用进入内核与“切到另一线程”是不同过程；进入内核不一定发生线程切换。无竞争 mutex 通常可走用户态原子操作快路径；竞争时可能自旋或使用 futex 等机制睡眠等待，而不是每次 lock 都必然进入内核。[futex(7)](https://man7.org/linux/man-pages/man7/futex.7.html)

线程栈的虚拟空间、实际驻留物理页、线程管理对象是不同成本。不要用固定“线程创建多少微秒”或“一个线程总占 8 MiB 物理内存”推断容量。

### 0.3 数据竞争不只是“偶尔丢一次加法”

```cpp
// 错误示意，不是建议运行的实验。
int count = 0;
// 线程 A：++count;
// 线程 B：++count;
```

普通 ++ 涉及读、计算和写。更重要的是，C++ 中无同步的并发冲突访问（至少一方写，且不全是原子操作）构成数据竞争，行为未定义。它不保证仅仅得到较小的 count；编译器可基于语言规则作优化。[C++20 数据竞争规则](https://timsong-cpp.github.io/cppwp/n4861/intro.races)

正确处理方式可以是：用同一 mutex 保护全部读写；简单独立计数用 atomic；每个线程只操作自己的一份数据，汇总前 join。`volatile` 不能提供线程间同步，sleep 也不是同步协议。

### 0.4 内存可见性与 happens-before

两个线程看到的读写顺序，需要由语言与同步操作共同规定，不是“机器 RAM 是共享的所以自然都能看到”。编译器、CPU 缓存和执行顺序都会参与实现。

例如 mutex 解锁与后续成功获取该 mutex 建立同步；join 成功后，主线程可以读取目标线程完成前写入的结果。release/acquire 原子操作也能发布其他数据，条件是接收端确实观察到相应发布操作。

术语 **happens-before** 表示由语言规则建立的先后/可见性关系，不是用钟表测量“这句在几毫秒前运行”。本篇优先用 mutex、join、条件变量和 semaphore，先不用复杂弱内存序算法。

### 0.5 共享 fd 的 I/O 风险

同进程线程共享 fd 表：A 关闭 fd，B 可能仍准备使用那个编号；内核复用编号后，B 的下一次调用甚至可能操作另一个对象。把 fd 放进 atomic<int> 并不能解决其生命周期。

多个线程同时 recv 同一个 TCP socket，会争夺字节流；同时 send 多个消息可能让应用层消息顺序/边界难以管理。常见设计是一个 I/O 线程或连接执行器拥有 socket，业务线程只处理请求对象，完成后把响应交回所有者。

不要在持有共享业务锁时执行可能很慢的 read/send、数据库操作或等待另一个线程，以免长时间阻止别人取得进展。

### 0.6 读懂 API 和语法

- `pthread_t`：pthread 线程标识，属于实现定义/不透明类型，不是可移植的 unsigned long。
- `void *(*function)(void *)`：线程入口接受通用指针并返回通用指针。
- `&argument`：传对象地址；入口再转换回真正的结构类型。
- `pthread_*` 大多数接口：0 成功，非零值本身是错误码，不能直接假设 errno 已设置。
- `sem_*`：通常是 0 成功、-1 失败并设置 errno，sem_wait 可被信号以 EINTR 中断。
- C++ `std::lock_guard`：作用域持锁；`std::unique_lock`：允许解锁、移动及配合 wait。
- lambda `[&]`：引用外部对象，要求这些对象活到线程完成；`[&, i]`：其他对象引用，i 按值保存。

所有示例对线程入口参数都保留足够生命周期，并在返回 main 之前回收线程。

## 14.1 Linux 线程概述

**书中对照，PDF 第 505～510 页**：讲用户/内核线程的映射模型，比较历史 LinuxThreads 与 NPTL，再说明现代 Linux 的 1:1 POSIX 实现。

### 14.1.1 N:1、1:1 与 M:N

为避免书中 M/N 符号前后易混淆，这里采用“用户执行流数 : 内核可调度线程数”：

| 模型 | 含义 | 影响 |
|---|---|---|
| N:1 | 多个用户执行流放到一个 OS 线程 | 可在用户态切换；该 OS 线程阻塞会妨碍这些执行流，不能凭一个线程多核并行 |
| 1:1 | 一个应用线程对应一个内核调度实体 | 可多核执行，系统需要维护每个线程 |
| M:N | 多个用户执行流由运行库调度到多个 OS 线程 | 两级调度，需要运行库协调阻塞与任务迁移 |

现代 Linux pthread 通常采用 NPTL 的 1:1。这里“内核可调度线程”仍可以执行用户态程序，不能将它与始终执行内核服务代码的 kthread 混为一谈。

你刚学习的 C++20 协程是库/应用调度的可暂停任务；一个或多个 OS 线程可以承载多个协程。协程、线程和进程是不同层次。

### 14.1.2 NPTL、PID、TID 与 pthread_t

NPTL 是 glibc 的 POSIX 线程实现；Linux 用 task/clone 系列机制管理线程，按共享标志组织地址空间、fd 等资源。可以查询：

```bash
getconf GNU_LIBPTHREAD_VERSION
```

getpid 在同进程线程中返回相同进程 ID；Linux gettid 表示各自内核线程 ID；pthread_self 返回 pthread 标识，std::this_thread::get_id 返回标准 C++ 线程 ID。不能直接把它们当成同一种编号。比较两个 pthread_t 用 pthread_equal。

用 `ps -L` 或 `/proc/PID/task` 可观察多个线程。LinuxThreads 的旧限制用于理解历史，不应按它描述当前 NPTL 的主线程/信号/资源行为。[NPTL 文档](https://man7.org/linux/man-pages/man7/nptl.7.html)

## 14.2 创建线程和结束线程

**书中对照，PDF 第 511～514 页**：介绍 pthread_create、pthread_exit、pthread_join、pthread_cancel，以及取消状态与延迟/异步取消。

### 14.2.1 pthread_create

```c
pthread_t thread;
struct Argument argument = {21, 0};
int error = pthread_create(&thread, NULL, calculate, &argument);
```

入口 calculate 接收 `void *`，转回 Argument 指针，计算后可返回这个指针。不能返回入口函数栈上已结束的局部对象地址。创建成功之后，新线程与当前线程谁先继续运行，没有固定保证；不要靠 sleep 安排初始化。

错误可能来自线程/进程数量限制、内存、属性或权限等。使用 `strerror(error)` 解释 pthread 返回码，不直接 perror 旧 errno。pthread_t 的内部布局不要从头文件私有成员复制到自己的代码里。

### 14.2.2 退出与 join

pthread 入口正常 return 与 pthread_exit 结束该线程有相应等价效果，不需要每个普通入口都手工 pthread_exit。join 会等待目标结束并回收 joinable 线程的相关资源，且可以取得返回指针。

return main 或 exit 会结束整个进程；pthread_exit 只结束调用线程（最后一个线程结束时进程也结束）。生产 C++ 程序应明确 join/停止，不依赖“主线程先退出，别的线程自己善后”。

### 14.2.3 std::thread 与 std::jthread

C++11 使用 std::thread 表达同样的并发执行：

```cpp
int result = 0;
std::thread worker([&] { result = 42; });
worker.join();
std::cout << result; // join 后读取。
```

std::thread 对象析构时若仍 joinable，会调用 terminate，**即使底层线程已经运行完**也必须 join 或明确 detach。入口中异常若直接逃逸，同样导致 terminate；用 exception_ptr/future 等观察错误。

C++20 std::jthread 的析构通常 request_stop 然后 join；这是协作停止请求，不是强杀线程。worker 必须检查 token，或采用支持 token 的等待接口。普通阻塞 socket 调用不会因 request_stop 自动取消。[jthread 规则](https://timsong-cpp.github.io/cppwp/n4861/thread.jthread.class)

### 14.2.4 pthread_cancel 的边界

默认是允许取消、延迟取消，在线程到达取消点时响应。async cancellation 可在资源状态不一致时打断，普通业务不宜使用。pthread_cancel 成功只是请求已发出，不证明目标此刻已结束；仍需 join。

pthread_cond_wait 取消时按规则重新获得关联 mutex，再执行 cleanup handler，所以取消路径应负责解锁。`pthread_basics cancel` 用 started 谓词握手，再取消等待线程；cleanup handler 解锁，join 确认返回 PTHREAD_CANCELED。

C++ 项目通常更偏好停止标记/stop_token，而不是对 std::thread 的 native_handle 强行 pthread_cancel。某些实现用异常展开完成取消，不能随意吞掉底层取消机制，也不要认为它等同于普通 C++ exception。

## 14.3 线程属性

**书中对照，PDF 第 515～518 页**：介绍 pthread_attr_t 的初始化/销毁与 detachstate、栈、guard、调度策略/参数、继承调度和竞争范围。

### 14.3.1 属性对象与创建

```c
pthread_attr_t attr;
pthread_attr_init(&attr);
/* 设置所需属性 */
pthread_create(&thread, &attr, entry, argument);
pthread_attr_destroy(&attr);
```

属性在创建时使用；之后销毁 attr 不等于销毁线程。pthread_attr_t 是不透明对象，不能固定写死书中某架构的 36 字节布局。

### 14.3.2 joinable 和 detached

默认 joinable；detached 线程结束后自动回收相应线程资源，不能再 join。正确常量是 `PTHREAD_CREATE_DETACHED`。detach 不让线程脱离进程，也不延长参数/引用对象寿命。

若 detached 线程引用 main 的局部变量，main 返回或变量离开作用域时可能悬空。多数业务优先可管理的 joinable worker，不随意 detach。

### 14.3.3 栈与 guard

默认栈大小受平台/启动限制等影响；本机 lifecycle 查询并打印真实属性值，不把 8 MiB 当跨系统固定答案。stacksize 要满足最小值和平台要求。自定义 stack 地址时，分配、对齐、寿命、guard 等责任也增加。

guard 通常通过禁止访问的页面帮助发现栈越界；它不是“用来继续安全运行”的冗余空间。栈溢出可能终止进程，递归深度和大栈数组应谨慎。

### 14.3.4 调度相关属性

SCHED_OTHER 是常用策略，FIFO/RR 用于实时调度；调整权限受 capability、限制与配置影响，不能简化为只有 root 一种条件。实时线程若设计错误会妨碍其他任务，不要为了普通服务器性能随意提高优先级。

INHERIT_SCHED 与 EXPLICIT_SCHED 决定继承或采用设置值；Linux NPTL 主要支持 SYSTEM scope。标准 C++ 不统一这些 pthread 属性，可以在明确平台边界后借 native_handle 或直接 pthread API 配置，但要协调所有权。

## 14.4 POSIX 信号量

**书中对照，PDF 第 519～520 页**：sem_init/destroy/wait/trywait/post，用于控制资源数量与交接，区别于第 13 章 System V 信号量集。

| API | 行为 |
|---|---|
| sem_init(sem, pshared, value) | 初始化计数；进程内共享用 pshared=0 |
| sem_wait | 有资源则减一，没有则等；失败时检查 errno |
| sem_trywait | 不等待，没有资源可报 EAGAIN |
| sem_post | 增加计数，令等待者有机会继续 |
| sem_destroy | 使用者全部结束后销毁 |

信号量不是 SIGUSR1/SIGCHLD 这类信号。它不要求“获取者必须是释放者”；mutex 通常有锁持有者语义。计数初值 1 可以作互斥用途，但不意味着二者所有语义相同。

`pshared != 0` 时，同步对象还必须实际位于进程共享映射，光设一个参数不足以跨进程同步。

### 单槽生产者/消费者

实验使用两个 semaphore：empty 初值 1，full 初值 0。

```text
producer：wait(empty) → 写 value → post(full)
consumer：wait(full)  → 读 value → post(empty)
```

生产者写 1～100；消费者每次取一项，和应为 5050。token 交接保证不会在旧 value 还没读完时覆盖下一项。若复杂队列有多个生产者/消费者，还需保护容器操作，不能只给容量计数就假设整个 std::queue 安全。

C++20 `std::counting_semaphore<N>` 的 acquire/release/try_acquire 对应类似资源计数思想；`std::binary_semaphore` 是上限类型参数为 1 的别名。标准设施面向线程，没有标准跨进程共享保证，也没有自动响应 stop_token 的 acquire。

`modern_cpp20 semaphore` 演示四个任务共用两个许可，活动峰值最多为 2，线程不必按固定次序获得许可。

## 14.5 互斥锁

**书中对照，PDF 第 521～527 页**：介绍 init/destroy/lock/trylock/unlock、共享与类型属性，最后用两个锁的相反获取顺序展示死锁。

### 14.5.1 什么是临界区

保护的是一组共享状态的不变量，例如账户余额与转账记录。全部冲突读写要遵循同一同步协议；只锁写者、不锁读者，仍可能构成数据竞争。

```cpp
{
    std::lock_guard<std::mutex> lock(mutex);
    ++counter;
} // 自动 unlock，包括正常异常展开路径。
```

C 代码显式 lock/unlock，pthread lock 大部分返回非零错误码；C++ RAII 能减少早退/异常遗漏 unlock，但不免除锁粒度、顺序和数据生命周期设计。

| pthread 接口 | 用途与注意事项 |
|---|---|
| pthread_mutex_init(&mutex, attr) | 初始化；attr 为 NULL 采用默认属性 |
| pthread_mutex_lock(&mutex) | 取得锁，不满足条件时可能等待 |
| pthread_mutex_trylock(&mutex) | 立即尝试，锁忙时可返回 EBUSY，而不是设置 errno |
| pthread_mutex_unlock(&mutex) | 按所属线程和锁类型规则释放 |
| pthread_mutex_destroy(&mutex) | 没有使用者后销毁，不负责帮忙停止其他线程 |

静态初始化可使用 PTHREAD_MUTEX_INITIALIZER，不应对已经初始化的同一对象再次 init。正在持锁/使用的对象不能任意复制或销毁；std::mutex 本身也禁止复制。

### 14.5.2 mutex 类型与属性

| 类型/属性 | 重点 |
|---|---|
| NORMAL | 不允许随意重复 lock/unlock，通常不会帮你检查所有误用 |
| ERRORCHECK | 可报告同线程重复加锁等误用，适合部分诊断 |
| RECURSIVE | 同线程可重复取得，需要等次数 unlock |
| DEFAULT | 行为由实现映射/规则决定，不把它视为通用检错锁 |
| PROCESS_SHARED | 配合共享存储用于进程间；普通 std::mutex 没有此标准保证 |

普通 mutex 不保证 FIFO、公平或锁竞争一定按优先级解决。recursive mutex 也不是避免所有死锁的通用办法。需要崩溃后恢复时，robust mutex 还要求处理 owner-dead、修复状态等协议。

### C++14/17 的共享锁

C++14 提供 shared_timed_mutex、shared_lock；C++17 提供 shared_mutex。多个只读者可同时共享锁，写者必须独占锁。shared_lock 下不能修改同一共享对象。

读多不必然意味着共享锁更快，锁元数据、cache、写者饥饿和工作量都影响结果。本目录同一份 shared_mutex_demo 分别以 C++14/17 构建，两个写者最终累计 2000，reader snapshot 随调度变化。

### 14.5.3 死锁与避免

```text
线程 A：持有 lock1，等待 lock2
线程 B：持有 lock2，等待 lock1
→ 双方都无法继续释放已持有的锁
```

原书用 sleep 扩大出现这种状态的机会；sleep 不能保证精确时序，也不是修复手段。一般可规定全局锁顺序，或用多锁算法一次获取。

C++11 可配合 defer_lock 的 unique_lock 和 std::lock；C++17 常用 `std::scoped_lock(a.mutex, b.mutex)`。同一 mutex 不能在这里作为两个不同锁重复传入；它也不能消除资源/条件等待造成的其他循环依赖。

本例 scoped_lock_cpp17 让两个线程做相反方向转账，不故意制造永久死锁，结束时 A/B 均为 1000。

## 14.6 条件变量

**书中对照，PDF 第 528～530 页**：讲 cond init/destroy/wait/signal/broadcast，wait 必须持有关联 mutex，等待时释放，返回前重新取得。

### 14.6.1 通知不是数据，也不保存过去的通知

条件变量不是一个存储“通知次数”的计数器。notify/signal 可能发生在别人开始 wait 之前，此时没有现成 waiter 需要唤醒；等待者后来不能靠一个没有保存的通知知道当前条件。

真正的状态是 predicate，例如 `ready`、`!queue.empty()`、`closed`，必须和相关数据用同一 mutex 保护。

```cpp
std::unique_lock<std::mutex> lock(mutex);
cv.wait(lock, [&] { return ready; });
```

等价思想：持锁时循环检查，若 false 则 wait，醒来并重新持锁后再检查。即使 spurious wakeup，或别的消费者先取走了数据，也不会错误前进。

### 14.6.2 原子释放并等待的意义

wait 将“释放锁并进入等待”与匹配通知协调，使遵循同一 mutex 谓词协议时，不会在检查到 false 与进入等待之间漏掉必须观察的状态变更。它不是“永远不会错过任何通知”的万能保证。

```text
producer：lock → 修改 data/ready → unlock → notify
consumer：lock → ready? false → wait(释放锁)
                         ← 被唤醒并重新获得锁 ←
            ready? true → 使用 data
```

notify 后等待线程仍需要取得 mutex，不能假定它立刻运行。notify_one 不指定某个唯一线程；要唤醒全部等待者或停止一组 worker，用 notify_all 并各自检查条件。

需要期限时使用 pthread_cond_timedwait 或 C++ wait_until/wait_for，并且超时后仍要在持锁状态下判断谓词。一次任务的截止时间不要在每次虚假唤醒后从头刷新。POSIX 默认时钟与属性有关，可按支持情况配置 CLOCK_MONOTONIC；C++ 任务期限通常优先采用 steady_clock 表达经过时间。

### 14.6.3 有界队列与退出

本目录 BoundedQueue 的 not_empty 谓词为 `closed || !values.empty()`；not_full 为 `closed || size < capacity`。close 持锁设置 closed，再通知两边。消费者可排空现有数据后返回 false；生产者关闭后拒绝加入。

这是完整的“数据 + 谓词 + 通知”关系，也展示背压：队列满时上游等空槽，避免无限增长。它仍是阻塞式队列，不应未经改造用在必须及时返回的 I/O event loop 中。

销毁 mutex/condition_variable/semaphore 前，全部使用者必须退出等待。不要依赖书中提到的 EBUSY 返回来当安全销毁检测，错误销毁本身可能是未定义行为。

## 14.7 线程同步机制包装类

**书中对照，PDF 第 531～533 页**：将信号量、互斥锁、条件变量封装在 locker.h，构造初始化、析构销毁，并处理部分初始化失败。

### 14.7.1 封装应表达所有权和操作协议

现代 C++ 应做到：禁止误复制同步对象；构造失败保留具体错误；析构不抛出；确保没有正在等待/运行的使用者；用作用域锁而不只暴露散落 lock/unlock。

POSIX errorcode 可以封装为 system_error，sem 的 EINTR 可按停止策略重试。很多通用项目优先直接采用 std::mutex/condition_variable，避免重复实现基础设施；需要特殊 pthread 属性或进程共享时再封装原生接口。

### 14.7.2 原书 cond 封装为何还不够

仅在 cond 对象里面放一个私有 mutex，然后提供无参数 wait() 和 signal()，不能让调用方把“检查队列状态”和“进入等待”置于同一临界区；它也没表达谓词循环。这样业务状态与等待协议分离，可能产生漏通知/错误唤醒。

本例把 mutex、队列、closed 标志与 condition_variable 放在同一个 BoundedQueue 对象中，由 push/pop/close 维护不变量。

### 14.7.3 线程所有者与销毁顺序

C++11 ThreadGroup 析构 join 所有已创建线程，可处理部分线程创建失败，但 join 可能阻塞。调用者必须先关闭队列/停止工作，否则 worker 永远 wait，析构也无法完成。

代码中共享数据先声明、ThreadGroup 后声明，使线程所有者在反向销毁时先 join，共享对象再结束。遇到 producer 异常，先 queue.close 唤醒消费者，再展开线程所有者。

`shared_ptr` 可以协调某些对象的寿命，但不会自动锁住对象的成员；同一个 shared_ptr 变量的并发读写也有独立规则。内存管理线程安全与业务数据线程安全要区分。

## 14.8 多线程环境

**书中对照，PDF 第 534～540 页**：介绍库函数的可重入版本、多线程 fork 后继承锁问题、pthread_atfork，以及预先屏蔽信号并由一个线程 sigwait。

### 14.8.1 线程安全、可重入与异步信号安全

| 概念 | 含义/例子 |
|---|---|
| 线程安全 | 多线程按文档约束同时调用仍正确；可能内部使用 mutex |
| 可重入 | 尚未完成时再次进入也可正确处理，不能简单依赖同一份全局临时状态 |
| 异步信号安全 | 可在信号处理器等受约束上下文安全调用，范围更严格 |

内部用锁的函数可以线程安全，却不适合信号处理器：信号可能打断正在持锁的同一线程，handler 再取得那把锁便死锁。因此不能把三者当同义词。

_r 接口常把内部状态或输出缓冲交给调用者，如 localtime_r、strtok_r；但不要认为“没 _r 就都不安全”。检查实际 man page 的 thread-safety 属性及额外限制，避免依赖静态输出的长期指针。

实验两个 tokenizer 各自使用输入数组和 saveptr，得到 3/4 个 token；它们没有共享 strtok 的隐式解析位置。

### 14.8.2 多线程 fork 与 atfork

fork 后 child 只保留调用者线程，却继承地址空间中的各种锁状态。父进程某个其他线程持有的私有锁，在 child 中可能没有能继续执行的拥有者。parent 稍后解锁也不会解开 child 的私有副本。

原书 deadlock 实例清楚展示这一风险，但 child 使用 printf、lock 等并不适合作为通用多线程 fork-after 代码。正常策略是尽快 exec，child 路径只调用异步信号安全函数；或采用 posix_spawn 等方案。

pthread_atfork 提供 prepare/parent/child hook，可协调一部分已知状态；它不能自动修复所有 allocator、C++ runtime 和第三方库的内部锁，也不能把任意 child-side pthread 操作变成异步信号安全。

`pthread_environment fork-exec` 在 parent 中存在另一个等待线程，child 不碰它继承的 semaphore/锁，不调用 stdio，而直接 execve，失败 _exit。parent 回收 child，再唤醒并 join worker。要进一步看 spawn，可参考 [第 13 章程序](../code/base_multi_process/readme.md)。

### 14.8.3 线程和信号

信号处理设置通常由进程共享，signal mask 按线程维护；新线程继承创建者当前 mask。进程定向信号可由合适的未屏蔽线程接收；线程定向信号针对指定线程；故障类信号通常与发生故障的执行线程相关。

推荐学习模式：

```text
main 先 pthread_sigmask 屏蔽 SIGUSR1 等信号
→ 创建 worker，它们继承 mask
→ 指定一个信号线程 sigwait
→ 收到后在正常线程上下文执行业务停止/通知
```

sigwait 同步取信号，返回后不是异步 handler 上下文，能更方便地使用锁/日志等。不要同时随意用 handler 和 sigwait 争抢同一信号。sigwait 返回0成功、非零errorcode，pthread_sigmask 也用 pthread 风格。

实验仅向自己创建的等待线程发 SIGUSR1，消费后 join 并恢复 main 原来的 mask，不向外部进程发信号。

pthread_kill 的默认终止信号动作仍可能结束整个进程，不是“只杀这个线程”。线程 join/detach 结束之后，其 pthread_t 可以失效/复用；`pthread_kill(thread,0)` 不能作为任意旧 handle 的可靠健康检查。

## 15. C++11～20 的实践选择

| 标准 | 常用设施 | 适用提示 |
|---|---|---|
| C++11 | thread、mutex、lock_guard、unique_lock、condition_variable、atomic、future | 多数线程同步的基础已具备 |
| C++14 | shared_timed_mutex、shared_lock | 读写共享锁与超时能力，先测收益 |
| C++17 | shared_mutex、scoped_lock | 读者并发、多个锁的作用域管理 |
| C++20 | jthread、stop_token、counting_semaphore、latch/barrier、atomic wait/notify | 停止、容量控制、阶段同步与避免纯轮询 |

### 15.1 协作停止

本例 jthread 使用 `condition_variable_any.wait(lock, stop_token, predicate)`，能响应停止请求。单纯 jthread 析构/请求停止不会自动唤醒普通 condition_variable 的无期限 wait，也不会取消阻塞 recv。生产服务要给等待明确出口。

### 15.2 atomic wait/notify 与内存发布

main 写普通 message 后 `state.store(1, release)`，再 notify_one；reader `state.wait(0, acquire)` 观察新值后才能读 message。notify 本身不是替代发布数据的魔法；必须有正确的原子状态与内存顺序协议。

单次从0到1的演示容易理解，通用队列会遇到 ABA/版本、多个消费者和竞争条件，不能直接复用这个布尔状态解决全部同步。

### 15.3 barrier 与 latch

barrier 可重复按阶段汇合；latch 用于一次性倒计数门。示例中两线程各写不同数组元素，barrier 之后再读取两个元素之和，得到42。阶段结束建立的同步使此前结果对参与者可见；不要在别人还写同一元素时提前读。

### 15.4 I/O 事件循环与线程池

现代服务常由少量 I/O event loop 管理连接，工作池处理重计算或阻塞依赖。使用有界队列、每连接执行所有权、请求期限和关停协议；不要让 event loop 随意等一把长期占有的业务锁。

标准 C++20 没有通用线程池/executor 或网络事件循环。项目可以选成熟库或封装 POSIX，但核心仍是数据寿命、等待出口、错误传播与资源上限。协程可与线程组合，参见 [协程专题](base_coroutines.md)。

## 16. 原书表述与现代理解的区别

| 原书主题 | 需要注意 |
|---|---|
| 内核线程都“在内核空间运行” | 内核调度的用户线程与内核服务 kthread 应区别 |
| pthread_t 固定 unsigned long、attr 固定布局 | 这是特定实现/架构，不是可移植 API 保证 |
| 每线程栈默认8 MiB | 查询实际属性，区分虚拟预留和物理驻留 |
| 普通 mutex 保证公平 | 不保证 FIFO/公平性；读写锁也需测竞争与饥饿 |
| 条件变量不会错过任何变化 | 正确谓词/mutex 协议是前提；通知不存储 |
| cond/mutex destroy 可通过 EBUSY 安全判断 | 使用中销毁可能未定义；先退出并 join |
| 可重入就是线程安全 | 与线程安全、signal-safety 分别分析 |
| atfork 使 child 可以任意继续业务 | 不能绕过 fork-after 的异步信号安全限制 |
| pthread_kill(旧ID,0) 检测存在 | handle 寿命结束后不能随意再用 |

## 17. 调试与复习

编译和运行命令在 [README](../code/base_multi_thread/readme.md)。调试时可用：

```bash
ps -L -p PID -o pid,tid,stat,comm
strace -f -e futex,clone,clone3 ./modern_cpp11 queue
```

PID 是替换占位值；短实验可能退出太快来不及 ps。futex 系统调用数量也不是“加锁调用次数”，无竞争快路径可能不进内核。

ThreadSanitizer 可辅助发现覆盖到的数据竞争，但工具支持、运行环境、未执行路径和外部库都会限制结论；它不能证明不存在死锁/业务竞态。

试着解释：join 为什么让主线程能读结果？线程入口传引用为什么需要管理寿命？notify 为什么不能代替 ready？stop_token 为什么不会直接强杀线程？sem_post 为什么能由另一个线程调用而 mutex 不能随便跨线程解锁？fork 后其他线程持有的锁为什么危险？

资料：

- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html)、[pthread_create(3)](https://www.man7.org/linux/man-pages/man3/pthread_create.3.html)。
- [pthread_cancel(3)](https://man7.org/linux/man-pages/man3/pthread_cancel.3.html)、[条件变量接口](https://man7.org/linux/man-pages/man3/pthread_cond_wait.3.html)。
- [sigwait(3)](https://man7.org/linux/man-pages/man3/sigwait.3.html)、[NPTL](https://man7.org/linux/man-pages/man7/nptl.7.html)。
- [C++20 数据竞争规则](https://timsong-cpp.github.io/cppwp/n4861/intro.races)、[jthread](https://timsong-cpp.github.io/cppwp/n4861/thread.jthread.class)。

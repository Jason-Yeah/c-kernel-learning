# 多线程编程实验：POSIX 与 C++11～C++20

配套 [base_multi_thread.md](../../doc/base_multi_thread.md)，按游双《Linux 高性能服务器编程》第 14 章 14.1～14.8 的知识点安排。

先从 pthread 生命周期、mutex、condition/semaphore 学起，再对照 C++ RAII、读写锁、协作停止、atomic wait 和阶段同步。死锁示例展示正确的避免方案；默认运行不会故意一直卡住。

## 1. 文件与编译标准

| 源码/头文件 | 标准 | 内容 |
|---|---|---|
| `pthread_basics.c` | C17 / POSIX | 属性、创建/join、mutex、condition、semaphore、延迟取消 cleanup |
| `pthread_environment.c` | C17 / POSIX | strtok_r、sigwait、线程与 fork+exec |
| `modern_cpp11.cpp` | C++11 | mutex 计数、有界队列、线程异常传播 |
| `thread_group.hpp` | C++11 | 持有并 join 已创建线程 |
| `bounded_queue.hpp` | C++11 | mutex + 两个条件变量 + closed 谓词 |
| `shared_mutex_demo.cpp` | C++14/17 两种构建 | shared_timed_mutex / shared_mutex |
| `scoped_lock_cpp17.cpp` | C++17 | 相反方向转账的双 mutex 获取 |
| `modern_cpp20.cpp` | C++20 | jthread/stop_token、atomic wait、semaphore、barrier |

默认生成七个可执行程序，无第三方依赖。使用操作系统线程，不创建 System V IPC 对象，也不修改系统线程调度策略。

## 2. 编译与验证状态

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_multi_thread
make
```

需要 Linux、GCC/G++、Make。已在 GCC/G++ 13.3.0 下分别按 C17、C++11、C++14、C++17、C++20 无警告编译。`-pthread` 同时用于编译/链接线程相关支持，推荐按编译器接口使用，而不简单替换成一个手写链接库选项。

已运行全部 18 个模式，功能结果正确，均正常结束：计数、单槽交接、队列排空、取消清理、双锁转账、停止、原子发布、许可上限、barrier、信号和 fork+exec。

额外尝试了 ThreadSanitizer：一次有界队列检查未报告竞争；继续检查时工具因宿主地址空间布局报 `unexpected memory mapping`，包含非 PIE 尝试，无法完成整套动态竞态检查。这是工具运行限制，不能声称已通过全部 ThreadSanitizer 检查；功能验证也不是无数据竞争的形式证明。

## 3. 创建线程、参数和 join

```bash
./pthread_basics lifecycle
```

输出系统相关的默认 stack size，以及：

```text
joined: output=42 returned_same_argument=1
```

main 中的 Argument 一直存活到 join，入口函数 calculate 把 `void *` 转回它的地址，写 output=42，再返回该指针。join 后 main 读取 output，返回指针也还是那份合法对象。

线程入口没有返回局部临时变量地址；线程创建成功后 destroy(attr) 只是回收属性对象，不是回收线程。打印的 stack size 是属性中的虚拟栈尺寸，不是实际使用的物理 RAM 数量。

## 4. mutex 保护计数

```bash
./pthread_basics mutex
./modern_cpp11 counter
```

预期分别为 20000 和 40000。POSIX 示例两个线程各加 10000，C++11 四个线程各加 10000。

POSIX 用显式 pthread_mutex_lock/unlock；C++ 用 lock_guard 作用域保护。所有 ++ 都按同一同步协议访问 value。main join 完全部 worker 后再读取最终计数，避免它自己成为无同步读者。

不要通过删除锁、运行后“结果看起来还对”来判断没有竞争；未定义行为不是稳定实验手段。反例原理见主教程。

## 5. 条件变量：ready 和 value 的交接

```bash
./pthread_basics condition
```

预期 `condition reader received=42`。

main 持锁设置 value=42、ready=1，再通知；reader 持锁检查 ready，若 false 则 pthread_cond_wait。返回时它重新获得 mutex，并继续 while 检查。

即使 main 在 reader 开始 wait 前已设置 ready，reader 也能从共享状态判断无需等待。保证正确的是“同一个 mutex 下的状态协议”，不是 condition 会记住之前发送过的通知。

## 6. POSIX semaphore 单槽队列

```bash
./pthread_basics semaphore
```

预期 `semaphore handoff sum=5050 (expected 5050)`。

Slot 只有一个 value。empty=1 表示槽可写，full=0 表示尚无值：producer 获取 empty、写入、post full；main 获取 full、读值、post empty。1～100 不会被提前覆盖。

semaphore_wait 重试 EINTR，消费侧使用 5 秒绝对期限的 sem_timedwait，防止演示错误时无限等待。如果消费失败，会取消可能在 sem_wait 的生产者，再 join 和 destroy。正常演示没有取消。

这是单 producer/consumer 的交接，不是通用 MPMC 容器。多生产者下 std::queue 的内部操作仍需同步。

## 7. pthread 延迟取消与清理

```bash
./pthread_basics cancel
```

预期 `cancelled=1 (cleanup handler unlocks the mutex)`。

worker 取得 mutex 后安装 cleanup handler，设置 started 并 signal，再等待 finish。main 在同一 mutex 下等待 started，以确保已建立清理协议，再 pthread_cancel。

取消在线程的 cond_wait 取消点发生；POSIX 保证关联 mutex 会在执行 cleanup 前重新取得，所以 cleanup_unlock 释放它。join 得到 PTHREAD_CANCELED 后，main 才销毁 cond/mutex。

示例使用默认 deferred cancellation，不启用 asynchronous cancellation，也不取消外部线程。C++ 项目更常使用 stop_token/显式停止状态，不直接对 std::thread 的 native_handle 套用这段 C 取消逻辑。

## 8. C++11 有界队列与异常传播

```bash
./modern_cpp11 queue
./modern_cpp11 exception
```

输出：

```text
bounded queue sum=5050 closed and drained
main observed: worker failure
```

BoundedQueue 的容量为4，main 提交1～100，两个消费者分工累加。各自结果存独立数组元素，join 后 main 汇总。具体分配给谁不确定，但总和固定。

### 队列的三个谓词

- 可推入：`closed || size < capacity`；closed 时拒绝新值。
- 可弹出：`closed || !empty`；关闭但仍有数据时继续排空。
- 结束：closed 且 empty，pop 返回 false。

全部谓词、数据与 closed 使用同一个 mutex。push/pop 等待时用 unique_lock，完成后 unlock 再通知另一端。close 同时通知生产者与消费者，给阻塞线程明确出口。

异常模式在线程内部 catch，保存 exception_ptr；main join 后 rethrow 观察。不能让异常逃出 std::thread 入口，否则 terminate。join 只回收/同步，本身不会自动传播 worker 的业务异常。

### ThreadGroup 的边界

它只帮你 join，不会替你停止永远等待的线程。队列模式创建线程/生产异常时先 close，通知所有消费者，再展开 ThreadGroup。共享数据先构造、group 后构造，析构顺序保证 worker 先结束。

这不是完整线程池类，但队列和退出协议可作为第 15 章线程池学习基础。

## 9. C++14/17 读写锁

```bash
./shared_cpp14
./shared_cpp17
```

两者使用同一份源文件，按标准选择 `std::shared_timed_mutex` 或 `std::shared_mutex`。writer 用 unique_lock，reader 用 shared_lock。

预期 `shared mutex value=2000 reader_snapshot=...`。snapshot 是 reader 当时观察的值，可能小于 2000，不是错误；最终总值在 join 后才确定。

不要在共享读锁下修改同一份共享数据，也不能据一次运行推断读写锁一定比普通 mutex 快。写者饥饿、公平性和临界区工作量需要单独考虑。

## 10. C++17 双锁死锁预防

```bash
./scoped_lock_cpp17
```

输出 `deadlock avoided: A=1000 B=1000`。

两个线程分别执行 A→B 和 B→A 转账，每次 `std::scoped_lock(from.mutex, to.mutex)` 获取两把不同锁，避免手工顺序“先拿自己的第一把，再等第二把”形成环路。

示例不靠 sleep 排调度，不展示一个无法结束的死锁。scoped_lock 只解决这组锁获取协议；如果持锁后等其他资源，仍可能形成别的死锁。

## 11. C++20 四种同步能力

```bash
./modern_cpp20 stop
./modern_cpp20 atomic-wait
./modern_cpp20 semaphore
./modern_cpp20 barrier
```

### stop

worker 使用支持 stop_token 的 condition_variable_any wait。main request_stop，即使早于 worker wait 也会被观察到，再 join。

预期 `jthread cooperative stop observed=1`。jthread 不强杀线程；一个普通 cond.wait、sem.acquire 或 recv 的无期限等待不会自动因为 token 改变而结束。

### atomic-wait

main 先写 message，再 release-store state=1，notify_one；reader acquire-wait 观察新 state 后读 message。

预期 `atomic wait read: published payload`。message 本身不是 atomic，安全来自发布/获取协议；notify 不替代原子状态和内存序。

### semaphore

四个 worker 共享两个许可，许可获取后才计入 active。atomic 跟踪 peak，工作结束释放许可。

预期 `semaphore peak=1或2 (limit 2)`。当前调度通常为2，但测试只要求1～2，并不假定一定并行到峰值。

### barrier

两线程分别把数组元素写为20和22，等同一 barrier 阶段结束，再读取两项求和。

预期 `barrier phase sums=42,42`。如果第二线程创建失败，主线程 arrive_and_drop，避免第一线程永久等一个不会来的参与者。

## 12. 多线程环境：可重入、信号、fork

```bash
./pthread_environment reentrant
./pthread_environment signal
./pthread_environment fork-exec
```

### reentrant

两个 tokenizer 各有 input/saveptr，strtok_r 不共享隐式解析位置。join 后输出 `strtok_r independent token counts=3,4`。_r 名称是这类接口常见模式，但不是判断所有函数 thread-safety 的唯一标准。

### signal

main 先屏蔽 SIGUSR1，随后创建信号线程，继承屏蔽状态。main pthread_kill 只向该线程发送 SIGUSR1；sigwait 消费它并正常返回。join 后 main 恢复原 mask。

预期 `sigwait received SIGUSR1=1`。这里没有异步 signal handler，也不向外部进程发信号。实际服务可用信号线程触发队列关闭/停止 worker。

### fork-exec

parent 有一个 sem_wait 中的工作线程。fork 后 child 只执行预先准备好的 execve；失败 _exit，既不打印 stdio，也不碰继承的 mutex/semaphore。parent 回收 child，再 post gate 唤醒原 worker 并 join。

预期 `execve from multithreaded parent`。这个演示展示 child 路径的克制，不意味着所有多线程 fork 的库状态都因此没有风险；生产创建外部进程可优先考虑 posix_spawn。

## 13. 学习与调试

先看主教程的 OS/内存模型，再按编号运行。可以观察：

```bash
strace -f -e futex,clone,clone3 ./modern_cpp11 queue
```

系统调用数量与 std::mutex 调用次数不同。短实验执行很快，想观察线程 ID 可在学习副本加明确等待门，不靠 sleep 保证顺序。

ThreadSanitizer 的常用编译方式：

```bash
g++ -std=c++11 -O1 -g -pthread -fsanitize=thread modern_cpp11.cpp -o /tmp/threads_tsan
/tmp/threads_tsan queue
```

工具不能启动时报宿主映射错误，需在兼容环境使用；不应关掉检测后宣称通过。也不要用它当死锁/业务正确性的完整证明。

清理：

```bash
make clean
```

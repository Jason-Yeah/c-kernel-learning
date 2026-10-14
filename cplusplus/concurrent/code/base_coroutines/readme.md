# 协程配套程序：从手工状态机到 epoll、Asio

配套[协程入门教程](../../doc/base_coroutines.md)。按编号逐个学习，不用一开始就懂 promise_type 或自己编写通用调度库。

## 1. 程序与依赖

| 程序 | 源码 | 语言标准/依赖 | 目标 |
|---|---|---|---|
| `manual_fsm` | `01_manual_fsm_cpp11.cpp` | C++11，标准库 | 显式保存位置和数据 |
| `manual_resume` | `02_manual_resume_cpp20.cpp`、`task.hpp` | C++20，标准库 | 同线程暂停/恢复 |
| `generator` | `03_generator_cpp20.cpp` | C++20，标准库 | 用 co_yield 逐个产出 |
| `epoll_coroutines` | `04_epoll_coroutines.cpp`、`task.hpp` | C++20 / Linux | 非阻塞 socketpair + timerfd + epoll |
| `asio_echo`（可选） | `05_asio_echo_cpp20.cpp` | C++20 / Boost.Asio / pthread | 本机异步 TCP 回显 |

前四个程序默认构建，不需要第三方库。epoll 示例仅使用本机 Unix 域 socketpair，不访问外部网络或打开监听端口。可选 Asio 服务仅监听 127.0.0.1。

## 2. 编译和运行

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_coroutines
make
./manual_fsm
./manual_resume
./generator
./epoll_coroutines
./epoll_coroutines --cancel-demo
```

需要支持 `<coroutine>` 的编译器和标准库。本次默认示例用 GCC/G++ 13.3.0 编译和运行验证；`manual_fsm` 单独按 `-std=c++11` 构建，其余按 `-std=c++20`。

已检查 A/B 交错恢复、生成器 1～5 的输出、定时器与 socket 事件驱动四个任务完成，以及销毁等待任务后 epoll 注册归零。另用 AddressSanitizer/UndefinedBehaviorSanitizer 检查了局部对象析构、异常传播和 epoll 示例的销毁/取消路径，未发现问题。可选 Asio 示例的验证状态见第 7 节。

`make` 是构建工具；`-O2` 开启优化，`-Wall -Wextra -Wpedantic` 开启警告。可执行文件列在 `.gitignore` 中。清理只需 `make clean`。

某些受限环境允许创建 socketpair，却禁止 send，报错 `Operation not permitted`；应在允许本机 socket I/O 的 Linux 环境运行。它不表示协程语言机制无法使用。

## 3. 示例一：C++11 手工状态机

执行 `./manual_fsm`：

```text
A: first, value=10
B: first, value=10
A: second, value=11
B: second, value=11
```

Conversation 保存 name、value 和当前 State。第一次调用 resume 执行 first 分支，并保存 second 状态后返回。第二次调用从 second 分支继续。main 交替调用 A/B，产生交错执行。

这是手工状态机，不是语言级协程。你仍能看到“执行位置”“跨暂停的数据”必须被保存到对象成员。它与后续协程的输出一致，便于对比编译器替程序员做了什么。

## 4. 示例二：C++20 暂停与恢复

执行 `./manual_resume`，先打印：

```text
created: bodies have not run yet
```

之后输出与示例一相同。Task 的 initial_suspend 返回 suspend_always，因此 `conversation("A")` 只是建立帧并返回所有者，函数体第一次执行是在 a.resume()。

conversation 中的 value 需要跨挂起点保留。执行到 `co_await std::suspend_always{}` 时控制权返回 main；第二次恢复时从挂起点后继续，value 增加到 11，然后 co_return。

### task.hpp 逐项解释

- `Task::promise_type`：编译器根据返回类型寻找的定制类。
- `get_return_object`：构造外部 Task，关联同一协程帧。
- `initial_suspend`：定义 lazy start。
- `final_suspend`：协程完成后继续保留帧，等待所有者销毁。
- `return_void`：处理 co_return。
- `unhandled_exception`：存储异常，稍后 rethrow。
- `std::coroutine_handle`：访问 resume/done/destroy 的句柄；它本身不负责自动释放。
- Task 禁止复制、允许移动，析构 destroy，确保帧只有一个回收者。

**此 Task 不可 co_await。** 它只表示根协程所有权，没有实现 continuation、结果值、嵌套组合或多线程同步。后面 epoll 示例会用它拥有 root 任务，再借 handle 去登记等待。

resume 只能用于正确挂起的位置。示例二在 main 明确驱动每个步骤；示例四启动一次后由 loop 接管恢复，用户不能随意手动再次 resume 等待中的 I/O 任务。

## 5. 示例三：同步生成器

执行 `./generator` 输出 1～5。numbers 按值接收 count，循环 co_yield 当前值。IntGenerator::next 恢复到下一次产出，value 返回 promise 保存的整数。

- `yield_value` 保存产出值并返回 suspend_always。
- 初始挂起避免生成器一创建就算完。
- next 返回 false 表示完成；只有 next 成功后才允许 value。
- 迭代未完提前销毁也会 destroy 帧，清理活着的局部对象。

它没有 event loop 或额外线程。co_yield 是消费驱动的逐项计算，不是网络异步读写。本例 IntGenerator 为自定义类型；C++20 标准库还没有 C++23 的 std::generator。

## 6. 示例四：epoll 恢复协程

执行 `./epoll_coroutines`，通常输出：

```text
sender A: waiting for timer
sender B: waiting for timer
sender B: finished
receiver B: hello B
sender A: finished
receiver A: hello A
all four tasks completed on one thread
```

A 的定时延迟为 80 ms，B 为 30 ms；正常负载下 B 通常先输出，但不应把精确时间或顺序当实时保证。

### 每条数据经过什么地方

main 创建两对 socketpair，每对有 sender/receiver 两个端点。四个 Task 先各恢复一次：两个 receiver 因 recv 返回 EAGAIN 注册读等待；两个 sender 注册 timerfd 等待。然后 main 进入 epoll_wait。

timerfd 到期后 epoll 返回 token，loop 删除那次注册并 resume sender。sender 发送正文，然后 shutdown(SHUT_WR)。receiver 所等 fd 就绪时被恢复，再次调用 recv；读完正文并得到 EOF 后输出结果。

### Waiter 的三个成员

```cpp
bool await_ready() const noexcept { return false; }
void await_suspend(Task::Handle h) { token_ = loop_.watch(fd_, events_, h); }
void await_resume() const noexcept {}
```

本例等待就绪总由 epoll 处理，所以 await_ready 返回 false。await_suspend 将 handle 注册进 loop，当前协程暂停；await_resume 没有结果，只表示可以重新尝试系统调用。实际数据/错误由下一次 recv/send 读取。

### 为什么不会丢掉“刚好到达的数据”

recv 得到 EAGAIN 后，到登记 epoll 期间数据也可能到达。本例使用 level-triggered 注册：加入 epoll 时已经满足可读条件也会被观察到。因此不能只依赖某个提前查询的布尔变量；必须让实际 I/O 与 readiness 等待组合。

### 等待、取消与帧寿命

loop 持有的是借用的 Task::Handle；Task 对象才负责帧。每个 Waiter 记录自己的 token，销毁时将它从等待表注销。epoll 事件的用户数据放 token，防止 fd 编号复用后把旧事件恢复到新协程。

```bash
./epoll_coroutines --cancel-demo
```

输出：

```text
before task destruction: pending=1
after task destruction: pending=0
```

此实验建立一个永远不会得到正文的 socket 等待，随后销毁 Task；帧内 awaiter 的析构撤销等待。loop 必须一直活着，直到全部等待 Task 已销毁。

### 实现限制

只有一个线程；一个 fd 同时只能登记一个 waiter；没有真正完成式 I/O。网络读取仍是非阻塞 recv，只在 EAGAIN 时 co_await readiness。本目录演示按批次移除旧 token，支持单线程取消等待，不是可直接放到多线程生产工程中的运行库。

若任务在恢复后失败，loop 检查 promise 中的异常并重抛，外层销毁各 Task，同时撤销残留等待。5 秒无事件则报错，防止演示永久等住；它不代表完善的请求期限机制。

## 7. 可选：Boost.Asio TCP echo

当前环境没有安装 Boost 头文件，默认 make 不编译这个示例；此文件未在本次环境编译或运行。安装/提供兼容的 Boost.Asio 后可执行：

```bash
make asio_echo
./asio_echo 9020
```

Makefile 使用 `BOOST_ERROR_CODE_HEADER_ONLY` 和 pthread。编译器、Boost 和其 C++20 coroutine 支持必须匹配；对老项目请查实际版本文档。可从 [Asio 官方协程说明](https://www.boost.org/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html)核对 awaitable、co_spawn、use_awaitable 用法。

另一个终端用 Python 标准库：

```bash
python3 - <<'PY'
import socket
with socket.create_connection(('127.0.0.1', 9020), timeout=3) as s:
    text = b'hello coroutine\n'
    s.sendall(text)
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

程序只回显字节，不把一次 async_read_some 当成完整消息。async_write 负责组合写出整个读到的缓冲范围；协程局部 array 活到异步写完成，避免临时内存悬空。

listen 协程等待新连接，每个会话以具名 echo_session 函数启动，socket 按值移动到其帧。co_spawn 完成处理器报告异常。io.run 由一个线程驱动；多个连接遇到 I/O 等待时可以交替推进。

Ctrl+C/SIGTERM 会调用 io.stop 停止事件循环。这是演示退出策略，不是生产服务逐个 drain 客户请求的优雅关停方案。

## 8. 下一步练习

1. 比较示例一的 state/value 成员与示例二的局部变量、暂停点。
2. 把 suspend_always 改成 suspend_never，看第一轮恢复时会输出哪些阶段。
3. 在生成器中 yield 第三项后退出消费循环，解释帧如何回收。
4. 交换两个 sender 的定时延迟，确认恢复由事件决定，而不是按创建顺序轮转。
5. 跟踪 recv → EAGAIN → watch → epoll_wait → resume 的每一步。
6. 阅读 Asio 示例，区分业务协程与库里的 scheduler/promise 定制。

最重要的是明确四种职责：**函数保存计算状态，Task 拥有协程帧，awaiter 登记等待，event loop 决定何时恢复。**

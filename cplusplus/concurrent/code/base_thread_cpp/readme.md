# 现代 C++ 多线程短示例

配套 [base_thread_cpp.md](../../doc/base_thread_cpp.md)。每个文件独立运行，只展示一个问题；格式为四空格缩进、展开语句与清楚的作用域，不合并成一个大程序。

## 编译

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_thread_cpp
make
```

需要 GCC/G++ 与 Make。01/06 按 C++11，其余按 C++20 构建（主要为了使用 jthread）；mutex/condition 等基础 API 本身更早已存在。全部示例用 GCC 13.3.0 无警告编译，10 个程序的实际输出已检查。

## 按顺序运行

| 命令 | 输出 | 读代码时看什么 |
|---|---|---|
| `./thread_demo` | `result=42` | std::ref 传引用，join 后读结果 |
| `./jthread_stop` | `stop observed=1` | token 和可取消等待，request_stop 是请求 |
| `./mutex_demo` | `counter=20000` | lock_guard 自动解锁 |
| `./atomic_counter` | `atomic counter=20000` | 独立计数 fetch_add(relaxed) |
| `./condition_demo` | `received=42` | 同一 mutex 下的 ready/value 谓词 |
| `./future_demo` | `future result=42`、`caught: task failed` | get 返回结果，也重抛任务异常 |
| `./shared_lock` | `final=1000 snapshot=N` | 读锁/写锁，snapshot 随调度变化 |
| `./scoped_lock` | `A=1000 B=1000` | 同时取得两个不同 mutex |
| `./worker_queue` | `drained total=15` | 取任务后解锁、closed 放行、join |
| `./atomic_publish` | `read: 42` | release/acquire 发布普通 payload |

shared_lock 的 N 可以在0～1000之间，不能当最终值断言。没有示例依赖 sleep 来保证时序，等待顺序通过同步协议表达。

## 每个示例的边界

- **01**：一个 thread、显式 join；复杂 C++11 工程还需 RAII 管异常路径。
- **02**：condition_variable_any 的 stop-token 重载；普通 condition_variable 不能照搬此接口。
- **03**：mutex 保护的复合更新，main 在 join 后汇总。
- **04**：relaxed 适用于本例独立计数，不负责发布其他对象。
- **05**：保存 ready 状态；通知即使先发生，晚来的 reader 也检查得到状态。
- **06**：显式 launch::async；它不是通用线程池，也没有通用取消接口。
- **07**：shared_mutex 来自 C++17，线程所有者 jthread 来自 C++20。
- **08**：from/to 不同；scoped_lock 不处理锁外其他资源形成的死锁。
- **09**：一 worker、五个整数；展示正常排空流程，没有容量控制或通用任务接口。
- **10**：单轮 false→true 发布；多轮交接必须设计版本和数据所有权。

代码中线程主要负责计算，输出在 main 的 join/get 后进行，避免多线程终端输出混在一起影响初学阅读。

清理生成程序：

```bash
make clean
```

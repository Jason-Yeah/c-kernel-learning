# 多进程编程配套实验（C17 / C++20）

对应 [base_multi_process.md](../../doc/base_multi_process.md)，按《Linux 高性能服务器编程》第 13 章 13.1～13.9 组织。先学进程创建/回收，再逐步理解 IPC 和 fd 传递。

所有 fork 示例都从单线程程序启动。child 路径不是供现有多线程服务任意复用的通用写法；多线程 fork 后有严格的异步信号安全约束，见主教程。

## 1. 文件与运行目标

| 可执行程序 | 源码 | 内容 |
|---|---|---|
| `process_basics` | `process_basics.c` | fork 私有数据、execv、waitpid、pipe 长度协议 |
| `sysv_ipc` | `sysv_ipc.c` | System V semaphore、shared memory、message queue |
| `posix_shared_memory` | `posix_shared_memory.cpp` | shm_open/mmap 与进程共享 POSIX semaphore |
| `pass_fd` | `pass_fd.cpp` | Unix socketpair 的 SCM_RIGHTS，传递临时文件引用 |
| `spawn_capture` | `spawn_capture.cpp` | posix_spawn 文件动作，捕获 child stdout |
| 通用头文件 | `process_support.hpp` | UniqueFd、短子进程回收者 Child |

C 文件按 C17，C++ 文件按 C++20 编译。大部分 RAII/错误处理能力源自 C++11，`std::exchange` 从 C++14 可用，`std::construct_at` 是这里使用的 C++20 能力。

## 2. 编译

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_multi_process
make
```

需要 Linux、GCC/G++ 和 GNU Make。System V IPC 和 POSIX shared memory 不需要 root，但容器/seccomp/沙箱可能禁用相应系统调用或禁止写 `/dev/shm`。这类权限错误不等同于编译失败。

Makefile 使用 `-std=c17/-std=c++20`、`-Wall -Wextra -Wpedantic`。POSIX shared memory/semaphore 示例链接 pthread 和 rt，兼容常见 Linux libc 的链接要求；没有额外 Boost 依赖。

本次用 GCC/G++ 13.3.0 编译，全部示例无警告。已运行实验路径，检查了输出和退出状态：共享计数为 2000，原私有变量不被 child 修改，fd 接收时带 CLOEXEC，spawn 正确捕获输出。也确认创建的 System V ID 在结束后消失，POSIX shm 名称已移除。

## 3. fork：父子私有变量

```bash
./process_basics fork
```

预期看到：

```text
child pid=... ppid=... value=99
reaped child=... exit=0
parent pid=... value=10 (private memory is unchanged)
```

pid 每次不同。child 只修改自己的普通内存，parent value 仍为 10；程序先 wait，再打印 parent，所以这段输出顺序是有意安排的，并不代表 OS 一定先调度 child。

fork 前 `fflush(NULL)` 刷新 C stdio 缓冲，避免复制未刷新的输出后两边重复打印。child 用 dprintf 输出后 `_exit`；它不执行 C++ 析构或 stdio 普通退出清理。

## 4. exec：替换 child 程序

```bash
./process_basics exec
```

child 执行 `/bin/echo`，输出 `hello from execv`。parent 用 reap 函数 waitpid 回收，打印退出码。

源码用两个可写 char 数组和以 NULL 结束的 argv，传给 execv。成功 exec 不会返回；错误分支只有失败时执行，最后 `_exit(127)`。程序路径固定，不经 shell 解析。

`reap` 先用 WIFEXITED/WIFSIGNALED 判别编码状态，再读取对应退出码或终止信号。不要把 status 整数直接当 exit code。

### waitpid 的非阻塞查询

```bash
./process_basics wait-nohang
```

输出 `before release: waitpid(WNOHANG)=0 (expected 0)`，随后 `after release: exit=23`。child 先等待控制 pipe 的 EOF，parent 查询时它肯定还未主动退出；parent 关闭写端后 child 才完成。0 表示本次没有可领取状态，23 是 child 的正常退出码，不是 waitpid 返回值。最终 parent 回收它。

## 5. pipe：带消息长度的单向传输

```bash
./process_basics pipe
```

预期：

```text
child received framed pipe message
reaped child=... exit=0
```

步骤是：

1. pipe 得到读端 fds[0]、写端 fds[1]，fork 后双方都继承。
2. parent 关闭读端，child 关闭写端。
3. parent 写入四字节大端长度，再写正文。
4. child 循环读完整字段、检查长度不超过 128，按实际长度输出。
5. parent 关闭写端并回收 child。

write_all/read_exact 按每次实际字节数推进，处理 EINTR。若正文中途 EOF，read_exact 报失败而不是输出半条“成功消息”。它是实验用阻塞 helper；不能拿到事件循环中不加改造地使用。

## 6. System V 信号量：保护共享计数

```bash
./sysv_ipc sem
```

预期包含：

```text
semid=N protected counter=2000 (expected 2000)
```

N 是非负 ID，0 也合法。counter 通过 MAP_SHARED|MAP_ANONYMOUS 放在共享映射中；父子各做 1000 次 ++，每次由 semaphore 的 P/V 保护。

- semget(IPC_PRIVATE, 1, 0600) 新建一个集合。
- semctl SETVAL 把初值设为 1。
- semop 的 `{0,-1,SEM_UNDO}` 获取，`{0,1,SEM_UNDO}` 释放。
- parent 等 child 退出后查询计数，munmap，再 semctl IPC_RMID。

普通 `int` 加一不是原子更新；这里 Linux semaphore 提供跨进程同步。SEM_UNDO 退出时按内核规则补偿计数，但不能回滚损坏的业务数据。

## 7. System V 共享内存：child 写，parent 读

```bash
./sysv_ipc shm
```

预期：

```text
shmid=N parent read: hello from System V shared memory
```

shmget 建立 128 字节段，shmat 映射进 parent。附加后先 IPC_RMID 标记删除，再 fork 继承映射。child 写入后 detach、退出；parent wait 完再读，所以本例用退出/等待安排了访问顺序。

它没有实现长期生产者消费者同步。若 parent 在 child 还在写时读取，必须增加信号量等协议。这里最后一个映射 detach 后段回收，不留下正常运行后的 System V 共享段。

## 8. POSIX 共享内存和通知信号量

```bash
./posix_shared_memory
```

预期：

```text
parent read: hello from POSIX shared memory
```

创建 `/ck_multi_process_<PID>` 名称，权限 0600，使用 O_EXCL 防止复用同名旧对象。获得 fd 后立即 unlink 名称；之后通过已打开 fd/mapping 和 fork 继承通信，不需要其他进程再按名字打开。

ftruncate 设置对象大小，mmap MAP_SHARED 建立映射，C++20 construct_at 在映射内构造 Shared。这个结构只放 sem_t 和固定 char 数组。

`sem_init(&ready, 1, 0)` 的 1 表示 process-shared；child 写正文后 sem_post，parent sem_timedwait 后读取。deadline 为 CLOCK_REALTIME 绝对时刻加 5 秒，避免实验永久挂住，仍要注意系统实时时钟跳变。

资源释放顺序：回收 child → sem_destroy → munmap。shm fd 在 mmap 后已经关闭；名字先前已 unlink。若要给无亲缘进程打开，必须保留名字到对方打开成功，并另外设计初始化/同步协议。

### C++ RAII 类型的用途

UniqueFd 只负责 close，不负责 munmap/unlink；Mapping 负责映射；NameOwner 负责自己创建的名称；SemaphoreOwner 负责 sem_destroy；Child 负责 wait。

本目录 Child 的 destructor 会阻塞等短 child，因此不能直接当生产事件循环的通用进程对象。真正 supervisor 要设计非阻塞回收、超时、信号与失败重启。

## 9. System V 消息队列

```bash
./sysv_ipc msg
```

预期：

```text
msqid=N type=7 payload_bytes=24 data=hello from message queue
```

消息开头必须是正 long type；msgsnd/msgrcv 的 msgsz 是后面 text 正文长度，不包含 type。代码发送 24 个正文字符，不额外发送字符串终止 NUL；parent 用 fwrite 和实际长度输出。

child 发送一条小消息后退出；parent 确认发送成功，再以类型 7 和 IPC_NOWAIT 接收。这种先 wait 只适用于本例小消息，不能推广到会把队列填满的发送任务。

msgrcv 成功返回正文长度，并非一律为 0；最后 msgctl IPC_RMID 删除队列。进程退出不会替你自动删除该对象。

## 10. 查询 IPC 与检查清理

```bash
ipcs -s
ipcs -m
ipcs -q
```

不同权限/namespace 下看到的内容可能不同。正常运行这些实验后，不应留下自己刚创建的对象。程序打印的 semid/shmid/msqid 可用于定位；ID 可能很快被系统再分配，不要隔很久之后只凭数字去删除别人的对象。

若实验被 SIGKILL，信号量或消息队列可能留下。只有明确对象归属和不再使用后，才针对该 ID 使用 ipcrm；不要删除整个系统的全部对象。POSIX shm 名称用 shm_unlink 管理，通常不能通过 ipcs 表查看。

## 11. Unix 域 socket 传递 fd

```bash
./pass_fd
```

预期：

```text
received fd=N CLOEXEC=1
hello via passed file descriptor
```

mkstemp 建立临时文件，立即 unlink 保留打开引用；写正文并 lseek 回 0。fork 后 child 用 sendmsg 的 SCM_RIGHTS 将文件引用发送给 parent。

parent 关闭原文件 fd，确认 child 成功退出后 recvmsg；Unix datagram 队列仍保持辅助数据中的文件引用。内核为 parent 安装新 fd，程序读取并打印正文，所有引用释放后临时文件也回收。

### 辅助数据布局

- 一字节普通 tag 为 'F'，并非零长度数组。
- `alignas(cmsghdr)` 保证控制缓冲对齐。
- 缓冲分配 `CMSG_SPACE(sizeof(int))`，header 长度用 CMSG_LEN。
- 发送端 memcpy fd 到 CMSG_DATA。
- 接收用 MSG_CMSG_CLOEXEC，检查 MSG_CTRUNC/MSG_TRUNC、header 类型和 fd 数量。
- 接收端先用 RAII 持有实际收到的 fd，再检查格式；不符合协议时也回收它们。

发一个普通整数不是传递 fd。SCM_RIGHTS 传的是内核对象引用，而两个 fd 的数值可以恰好相同。接收者能访问的能力也随 fd 一起转交，真实服务应认证对端与校验对象用途。

## 12. posix_spawn 捕获 stdout

```bash
./spawn_capture
```

预期：

```text
captured: hello from posix_spawn
```

parent 创建 CLOEXEC pipe，file_actions 在新程序里关闭读端、把写端 dup 到 stdout、关闭多余写 fd，然后 spawn 固定路径的 echo。parent 关闭自己的写端，读取到 EOF，再 wait。

这个方式把重定向动作作为启动参数，不在 parent 里修改 stdout，也不用自己在 fork 后调用一串 C++ 业务逻辑。posix_spawn 返回非零错误码时，直接用该值构造 system_error。

若 stdout/stderr 输出巨大，必须持续并发消费两路，否则 child 等 pipe 空间、parent 又 wait child 可能死锁。本例只有一行 stdout；生产封装应结合 poll/epoll、超时和有界缓存。

## 13. 建议练习

1. 区分 fork 演示中的私有 value 与 sem 演示中的共享 counter。
2. 跟踪 pipe 两端在 parent/child 中有哪些引用，解释忘关闭写端为什么没 EOF。
3. 理解 IPC_PRIVATE 的 key=0 为何可创建多个 ID，不代表权限私有。
4. 对照 System V 与 POSIX 共享内存的“创建/附加/分离/删除名字”四个阶段。
5. 解释 message 正文为什么不含 long type，且可以没有 NUL。
6. 解释 pass_fd 为什么在 child 已退出、parent 已关闭原 fd 后仍能读取。
7. 对比 fork+exec 与 posix_spawn 的 child 路径风险。

清理构建产物：

```bash
make clean
```

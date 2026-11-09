# Linux 多进程编程：fork、exec、IPC 与现代 C++

本文按游双《Linux 高性能服务器编程》第二篇第 **13 章“多进程编程”** 的 13.1～13.9 编排。I/O 复用是第 9 章；本章讲进程创建与回收，以及进程间通信（IPC）。配套程序位于 [base_multi_process](../code/base_multi_process/readme.md)。

**原文依据**：已读取你提供的书籍 PDF 第 451～504 页（阅读器从 1 开始的页序号，不是纸质页码）。以下“书中对照”用自己的话概括，再补充当前 Linux 接口行为与 C++11～20 实践；配套代码是新编写的实验，不是整段复制原书程序。

你提到的 C11-C20，这里分为 **C11/C17** 与 **C++11/14/17/20** 两条标准线。Linux 多进程 API 属于 POSIX/Linux 接口，C 和 C++ 都可调用；现代 C++ 主要改善资源所有权、错误表达、数据结构与工程组织。

## 阅读路线与章节对照

先看第 0 节的内存与 fd 模型，再逐节阅读。优先运行 fork、exec、pipe 三个基础实验；理解之后再运行 System V IPC、POSIX 共享内存、fd 传递和 spawn。

| 原书小节 | PDF 页码 | 重点 | 配套实验 |
|---|---:|---|---|
| 13.1 | 451～453 | fork 与写时复制 | `process_basics fork` |
| 13.2 | 454～455 | exec 函数族与继承 fd | `process_basics exec`、`spawn_capture` |
| 13.3 | 456～458 | wait/waitpid、SIGCHLD、僵尸 | `process_basics wait-nohang`；所有子进程均回收 |
| 13.4 | 459～461 | pipe、socketpair 与端点关闭 | `process_basics pipe` |
| 13.5 | 462～475 | P/V、semget/semop/semctl、IPC_PRIVATE | `sysv_ipc sem` |
| 13.6 | 476～493 | System V/POSIX 共享内存、聊天室实例 | `sysv_ipc shm`、`posix_shared_memory` |
| 13.7 | 494～499 | 消息类型、msgget/msgsnd/msgrcv/msgctl | `sysv_ipc msg` |
| 13.8 | 500～501 | ipcs/ipcrm | 查询并识别本实验资源 |
| 13.9 | 502～504 | Unix 域 socket 的 SCM_RIGHTS | `pass_fd` |

## 0. 先建立 OS、I/O 和内存模型

### 0.1 进程与线程

进程是程序运行时的资源与地址空间容器；Linux 实际调度的是 task/线程。一个进程至少有一个线程。不同进程通常具有不同虚拟地址空间，同一进程的线程通常共享地址空间。

因此，“两个进程中的变量地址一样”不等于“指向同一份数据”。虚拟地址通过各自页表映射到物理页；fork 后地址布局相似，私有数据仍按进程隔离。共享内存则显式让两个进程的一段虚拟地址映射到相同底层存储。

```text
进程 A 虚拟地址 ──页表 A──> 物理页 P（私有）
进程 B 虚拟地址 ──页表 B──> 物理页 Q（私有）

进程 A 共享映射 ──页表 A──┐
                         ├──> 相同共享页 S
进程 B 共享映射 ──页表 B──┘
```

同一共享区域在不同进程可以映到不同地址。不要在里面保存只对某个进程有效的指针；存相对于区域起点的偏移、长度或固定布局字段更容易跨进程解释。

### 0.2 fd 与打开文件描述不是一回事

fd 是当前进程描述符表的索引。内核的 open file description 保存文件偏移、状态标志与底层对象引用。fork 后父子进程各有 fd 表，但对应项通常引用同一个 open file description。

```text
父进程 fd 3 ──┐
              ├──> 同一 open file description → 文件/socket/pipe 端
子进程 fd 3 ──┘
```

结果是：关闭一份 fd 不必关闭整个底层对象；普通文件当前偏移可能共享，`O_NONBLOCK` 等状态也可共享。`FD_CLOEXEC` 是 fd 自身标志，控制 exec 时的关闭行为。

IPC 的对象 ID（semid、shmid、msqid）不是普通 fd，不能用 close 删除；要调用各自的控制接口。成功 ID 也可能是 **0**，失败是 -1，不能用 `id > 0` 判断成功。

### 0.3 数据传输与共享

| 方式 | 数据大致路径 | 边界与限制 |
|---|---|---|
| pipe/Unix stream socket | 用户内存 → 内核缓冲 → 对方用户内存 | 字节流，需自行分帧 |
| System V 消息队列 | 用户正文 → 内核消息存储 → 接收者正文 | 保留消息及类型，有容量限制 |
| 共享内存 | 两边直接访问同一底层区域 | 不负责并发同步与消息边界 |
| SCM_RIGHTS | 传递内核对象引用，在接收端安装新 fd | 数据正文与辅助信息分开 |

共享内存省去反复通过内核搬运大块正文的步骤，但建立映射、同步、CPU cache 一致性和跨核访问仍有成本。不能直接宣称它在所有负载下都最快。

### 0.4 本篇 C/C++ 语法

- `pid_t`：进程 ID 类型，fork 返回值也用它。
- `&status`：取地址，waitpid 通过它写回退出状态。
- `struct ...`：多个字段组成的类型；`.` 访问成员，`->` 通过指针访问。
- `size_t`：非负大小；`ssize_t`：可表示 -1 的字节计数。
- `errno`：调用失败后解释原因；成功时不要根据旧 errno 判断结果。
- `EINTR`：调用被信号打断，可按停止策略重新尝试。
- `IPC_RMID`：删除/标记删除某个 IPC 对象，具体行为依对象类型。
- C++ `::fork`：明确在全局作用域查找系统函数。
- C++ RAII：对象负责资源，作用域结束时自动释放；子进程用 `_exit` 时不会执行普通 C++ 析构。

## 13.1 fork 系统调用

**书中对照，PDF 第 451～453 页**：讲一次 fork 在父子两边产生不同返回值，子进程拥有类似地址空间、继承打开的 fd，并以 COW 减少立即复制大块内存的成本。

```c
pid_t pid = fork();
if (pid < 0) {
    /* 创建失败，没有子进程 */
} else if (pid == 0) {
    /* 子进程从这里继续 */
} else {
    /* 父进程中 pid 是新子进程 PID */
}
```

### 13.1.1 从哪里开始执行

子进程从 fork 返回的位置继续执行，不是重新从 main 开始，也不是把 main 再调用一次。父子后续都执行同一份代码，所以必须分支。

失败可能来自用户/cgroup 进程数上限、内存压力等，应处理 `EAGAIN/ENOMEM` 等错误。多个线程参与创建/管理子进程时，还要规划谁负责回收和任务跟踪。

### 13.1.2 写时复制 COW

fork 后，普通私有内存可暂时引用相同物理页，并通过页表保护维持“各自私有”的语义。某一方尝试写入时触发写保护缺页处理，内核按需要分离该页；写者之后操作自己的数据。

复制的粒度是页面级别，不是只复制一个 C++ 变量。具体优化与已有共享情况有关，不是每次写都必然再复制一页。fork 也不是完全免费：页表、任务管理、文件引用等仍有开销。

实验 `process_basics fork`：子进程将 value 改为 99，父进程等子结束后仍看到 10。它验证的是私有内存语义，不会直接测量物理页复制数量。

### 13.1.3 哪些属性继承，哪些不同

新进程具有自己的 PID，PPID 指向父进程；打开 fd 的引用通常继承，工作目录也继承。**信号处理器设置和信号掩码通常继承，但子进程的待处理信号集合初始为空。** 原书把这几种信号状态混在一起的表述应区别理解。[fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)明确列出相关规则。

MAP_SHARED 映射属于显式共享，不能简单套用“子进程写任何数据都不影响父进程”的私有内存结论。

### 13.1.4 多线程程序 fork 的陷阱

多线程父进程 fork 后，子进程只保留调用 fork 的那个线程，其他线程不会一起继续，但它们留下的 mutex/库内部状态可能仍在内存中。某个锁被已消失线程持有，子进程再次尝试使用就可能死锁。

因此多线程 fork 后、exec 前只能调用异步信号安全的函数。不要在该子路径随意使用 iostream、malloc/new、复杂日志或 C++ 锁。`pthread_atfork` 可协调一部分已知锁，不能任意修复所有第三方库。[fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)

本目录的 fork 实验都从单线程程序创建子进程；不能原样搬到已经运行线程池的服务中。

## 13.2 exec 系列系统调用

**书中对照，PDF 第 454～455 页**：exec 将当前进程映像替换成指定程序，提供参数、PATH 查询与环境传递多种形式。成功后不会回到原程序调用点。

### 13.2.1 fork 创建，exec 替换

```text
父进程 fork → 子进程（相同代码）exec → 子进程运行新程序
          └→ 父进程继续做自己的工作并最终 waitpid
```

exec 本身不新建一个子进程，PID 通常保持不变。它替换代码、数据、栈与地址空间等程序状态；一些进程属性继续存在，部分属性按 exec 规则重置。之前的共享内存附加/映射不应假设继续保留。

### 13.2.2 函数名字怎么记

| 名字部分 | 含义 | 例子 |
|---|---|---|
| `l` | 参数列表 | execl(path, arg0, arg1, ..., NULL) |
| `v` | 参数数组 vector | execv(path, argv) |
| `p` | 按 PATH 搜索文件名 | execvp(file, argv) |
| `e` | 显式环境数组 | execve(path, argv, envp) |

argv 最后一项必须是空指针；argv[0] 通常是程序名；`envp` 每项是 `NAME=value`，最后也以空指针结束。路径安全、环境变量与参数编码要作为启动协议设计，不能任意拼接外部输入成 shell 命令。

`process_basics exec` 让子进程执行 `/bin/echo`。若 exec 成功，后面的错误分支不会执行；失败则诊断并 `_exit(127)`。127 是本示例的约定，不能据此区分所有可能的子程序故障。

### 13.2.3 fd 与 CLOEXEC

exec 通常保留未设置 close-on-exec 的 fd，这让 `dup2` 到标准输入输出的 pipe 能成为新程序的 I/O。其他内部 fd 最好在创建时用 `O_CLOEXEC/SOCK_CLOEXEC` 等设置，减少多线程里“先创建再设置”期间泄漏的机会。

泄漏的 pipe 写端可能令另一进程永远读不到 EOF；泄漏监听 socket 可能令服务重启时端口仍被占用。

### 13.2.4 现代 C++ 中启动外部程序

常见选择是 `posix_spawn/posix_spawnp` 或成熟进程库。spawn 将程序启动、fd 重定向等以参数/文件动作表达，减少自行在 fork 后 child 路径执行 C++ 代码的需求。它仍需要读取输出、观察退出状态、回收和超时设计。

注意 `posix_spawn` 及不少相关接口返回的是**错误码本身**，不是简单的 -1/errno 模式。C++ 可以用这个返回值构造 `std::system_error`。[posix_spawn(3)](https://man7.org/linux/man-pages/man3/posix_spawn.3.html)

本例 `spawn_capture` 用 pipe 捕获 echo 的 stdout，不经过 shell，也不修改父进程的 stdout。

## 13.3 处理僵尸进程

**书中对照，PDF 第 456～458 页**：讲退出状态保留、wait/waitpid 与 SIGCHLD，并用循环 waitpid(WNOHANG) 回收已经结束的多个子进程。

### 13.3.1 僵尸和孤儿必须区分

**僵尸**：子进程已经结束，但退出状态尚未被父进程/回收者领取。它不再执行代码，普通地址空间大多已回收，但内核保留退出信息等记录。

**孤儿**：父进程结束时子进程仍在运行，它被合适的收养者接管，例如 namespace init 或 subreaper。它可以继续执行；并不因为父进程退出就成为僵尸。原书将孤儿阶段称为僵尸的段落需要修正。

僵尸已经退出，发送 SIGKILL 不会令它“再次退出”。应让其回收者 wait，或修复回收者的生命周期设计。

### 13.3.2 waitpid 返回值与 status

```c
int status;
pid_t result = waitpid(child_pid, &status, 0);
```

阻塞等待指定子进程；返回其 PID，失败返回 -1。`WNOHANG` 表示若目标还没有可领取状态就立即返回 0。正数不只代表“正常退出”，也可能取得信号终止等状态，取决于 options。

| 宏 | 检查含义 |
|---|---|
| `WIFEXITED(status)` | 是否正常退出 |
| `WEXITSTATUS(status)` | 正常退出时取 exit status，先检查上项 |
| `WIFSIGNALED(status)` | 是否被信号终止 |
| `WTERMSIG(status)` | 取终止信号，先检查上项 |

status 是编码信息，不是直接的退出码。`waitpid` 不会把成功子进程的 errno 复制到父进程；若要传复杂错误，应设计 IPC 协议。[wait(2)](https://man7.org/linux/man-pages/man2/wait.2.html)

`process_basics wait-nohang` 先让 child 等待一个 pipe 的 EOF；parent 此时用 WNOHANG 查询，得到 0。parent 关闭最后的写端，child 被放行、以 23 退出，parent 再 wait 取得退出码。用明确的控制管道安排顺序，不靠 sleep 猜测调度，也不留下长期僵尸。

### 13.3.3 SIGCHLD 与事件循环

普通信号可合并，所以收到一次 SIGCHLD 不意味着只有一个子进程结束。处理时应循环：

```c
while (waitpid(-1, &status, WNOHANG) > 0) {
    /* 记录已结束的子进程；在信号 handler 中只能做异步信号安全的工作 */
}
```

直接信号 handler 要保存/恢复 errno，谨慎处理 EINTR，并避免 printf、new 等不安全操作。现代 Linux 服务也可用 self-pipe/signalfd 将信号接到事件循环，或使用 pidfd 等 Linux 特定机制观察进程；信号通知后仍要明确回收职责。

本目录短实验使用阻塞 waitpid，便于完整观察；C++ `Child` 的析构也会等待，所以不能把这种可能阻塞的 destructor 放在生产事件循环热路径里。

## 13.4 管道

**书中对照，PDF 第 459～461 页**：fork 后父子都持有 pipe 两端，关闭各自不用的端点以形成单向通信；双向可用两条 pipe 或 Unix socketpair。书中结合 Squid 日志子进程观察数据传递。

```text
父进程：关闭读端，保留写端 → pipe 内核缓冲 → 子进程：关闭写端，保留读端
```

pipe 是有限容量字节流。所有写端引用消失、现有数据被读完后，read 才返回 0。fork/dup 或 exec 泄漏的一份写引用就可能阻止 EOF。

所有读端关闭后写入会遇到 SIGPIPE/EPIPE。正数返回值只是实际处理的字节数，应处理短读写和 EINTR；非阻塞时还要处理 EAGAIN。

### 13.4.1 给 pipe 定义消息边界

实验 `process_basics pipe` 发送“四字节大端正文长度 + 正文”。子进程循环读满头部、校验不超过 128 字节，再循环读满正文。这和 TCP 分帧思路相同。

`PIPE_BUF` 的多写者原子性与“一次 read 读出一条消息”是两回事。写入具有不被其他写者穿插的条件，不代表读端保持调用边界。

### 13.4.2 无亲缘进程也能通信

匿名 pipe 常通过继承获得端点；也可以用 fd 传递交给原本无亲缘进程。命名 pipe（FIFO）用文件系统名字让无亲缘进程打开。Unix 域 socket 支持双向和 fd/凭据传递，适合作为多进程服务控制通道。

不要把普通 fd 数值发给另一个进程就当成共享资源，13.9 会解释真正的传递机制。

## 13.5 信号量

**书中对照，PDF 第 462～475 页**：解释临界区、P/V 原语，再依次介绍 System V 信号量集的 semget、semop、semctl 和 IPC_PRIVATE，最后以服务器进程共享一个信号量说明用途。

### 13.5.1 为什么共享变量加一需要同步

`++counter` 通常包含读、计算、写等步骤。两个进程可能都读到 10，再分别写 11，从而丢失一次增量。共享内存只共享字节，不提供原子更新或互斥。

信号量维护可用资源计数：P/等待获取资源，资源不够时可阻塞；V/释放增加计数并使等待者有机会继续。初值 1 常用于互斥；较大值可控制资源数量。信号量和异步信号（SIGCHLD 等）是不同概念。

原书关于“高级语言都没有这种原子操作”的表述不能用于今天的 C++：C++11 有 atomic/CAS 等。但跨进程的 `std::atomic` 不能无条件视为可移植 IPC；非 lock-free 实现可能依赖进程私有状态。这里使用 POSIX/Linux 明确提供的同步接口。

### 13.5.2 semget：创建信号量集

```c
int semid = semget(IPC_PRIVATE, 1, 0600);
```

返回非负对象 ID（可能为 0），失败为 -1；1 表示集合里一个信号量，0600 限制 owner 读写。创建后显式初始化，例如 `semctl(semid, 0, SETVAL, ...)` 设置初值 1，不依赖环境/可移植性假设。

与指定 key 配合时，`IPC_CREAT` 表示不存在则创建，`IPC_EXCL` 与它组合可检测已存在，避免两个初始化者混用对象。获取既有对象时需有“谁初始化、何时可用”的协议。

### 13.5.3 semop：原子操作一组计数

```c
struct sembuf operation = {0, -1, SEM_UNDO};
semop(semid, &operation, 1);
```

- `sem_num`：集合中信号量编号。
- `sem_op < 0`：按绝对值减少，当前值不足就等待。
- `sem_op > 0`：增加计数。
- `sem_op == 0`：等待值为 0。
- `IPC_NOWAIT`：不能立即完成时返回错误，不阻塞。
- `SEM_UNDO`：维护该进程的撤销调整，退出时按规则补偿。

一次 semop 提交的多项操作具有整体执行语义，但这不是对共享业务数据做事务。SEM_UNDO 不能修复已经写坏的共享记录，也不是死锁预防机制。P/V 要配对，异常路径也需释放；并处理 EINTR/EIDRM 等。[semop(2)](https://www.man7.org/linux/man-pages/man2/semop.2.html)

### 13.5.4 semctl：查询、初始化、删除

SETVAL/SETALL 初始化；GETVAL 查询；IPC_STAT 获取属性；IPC_RMID 删除集合。Linux/glibc 某些命令的第四参数使用调用方定义的 union semun，不能随意把指针/整数传错。

删除后，阻塞在此集合上的等待者可能以 EIDRM 返回。普通进程退出不会自动删除集合；指定一方负责 IPC_RMID。

### 13.5.5 IPC_PRIVATE 并不等于“只有自己能用”

它是特殊 key 值，要求创建新对象，不是隐私/访问权限开关。父子进程继承 ID 后可以共同使用；其他有权限且获得 ID 的进程也可能访问。权限来自 0600 等模式和内核凭据。

System V key/ID 在相应 IPC namespace 内解释，不是跨全系统/全部容器绝对唯一。相同 IPC_PRIVATE key 可创建多个不同 ID 的对象。

### 实验与现代 C++ 实践

`sysv_ipc sem` 用共享匿名 mmap 保存计数，父子各做 1000 次临界区增加，最终应为 2000。只有同步机制保护了“读-改-写”过程。

现代项目可选择 POSIX sem、配置 PTHREAD_PROCESS_SHARED 的 mutex，或服务器进程通过消息顺序处理请求。普通 std::mutex、C++20 std::counting_semaphore 没有标准跨进程共享保证。若进程可能崩溃，考虑 robust mutex 的 owner-dead 协议，但必须实际修复受保护数据再声明一致。

## 13.6 共享内存

**书中对照，PDF 第 476～493 页**：从 System V shmget/shmat/shmdt/shmctl 讲到 mmap/shm_open/shm_unlink，再把聊天室改为一连接一子进程：正文存共享区，父子用控制通道通知共享槽位置。

### 13.6.1 shmget：建立存储对象

```c
int shmid = shmget(IPC_PRIVATE, size, 0600);
```

返回共享段 ID，尚不是可以解引用的指针。创建新段时指定大小；实际页分配与内核内存策略有关，不能仅根据成功创建推断物理 RAM 都已就绪。

### 13.6.2 shmat/shmdt：附加与分离

```c
void *memory = shmat(shmid, NULL, 0);
if (memory == (void *)-1) { /* 失败 */ }
shmdt(memory);
```

shmat 将对象映射进当前进程空间；一般让内核选择地址。失败不是 nullptr，而是 `(void*)-1`。shmdt 移除此进程附加，不等于删除段；fork 通常继承附加映射。

原书展示了底层结构字段，开发时应包含系统头文件，不手抄结构布局或依赖私有成员类型。

### 13.6.3 shmctl 与删除时机

IPC_STAT 查询，IPC_RMID 标记删除；只要仍有有效附加，段可以继续存在，最后脱离后被回收。与信号量/消息队列的删除行为不同。

`sysv_ipc shm` 创建并附加后先标记删除，再 fork；父子靠已附加映射通信。这样即使之后异常退出，映射最后释放时也有自动清理路径。该实验基于 Linux 行为，不设计第三个进程在删除标记后重新按 ID 附加的协议。

子进程写完退出，父进程 wait 完才读取，因此这个小例子没有并发读写；不能据此认为长期共享内存无需同步。

### 13.6.4 POSIX 共享内存

```text
shm_open(name, O_CREAT|O_EXCL|O_RDWR, 0600)
  → ftruncate(fd, bytes)
  → mmap(... MAP_SHARED ... fd ...)
  → 读写和同步
  → munmap / close / shm_unlink
```

新对象长度初始是 0，必须先设定长度。Linux 常通过 /dev/shm 的 tmpfs 实现 POSIX 对象；共享内存名称不是普通相对路径。`shm_unlink` 移除名字，已有 fd/映射可继续存在；close 只释放 fd，不自动移除名字，也不等于 munmap。[shm_open(3)](https://man7.org/linux/man-pages/man3/shm_open.3.html)

`posix_shared_memory` 创建唯一名称后立即 unlink，靠后续 fork 继承映射。它在共享区放 `sem_t`，`sem_init(pshared=1, value=0)`；子进程写正文后 sem_post，父进程 sem_timedwait 再读取。同步对象必须真的位于共享映射中，不能把它放在普通私有堆里。[sem_init(3)](https://man7.org/linux/man-pages/man3/sem_init.3.html)

### C++ 对象与共享内存布局

共享区里直接放 std::string/std::vector 通常不行：内部指针可能指向创建进程的私有堆，另一进程无法解释。vtable、函数指针、智能指针控制块也涉及进程私有对象和地址布局。

示例使用固定 char 数组和 POSIX sem_t，并用 C++20 `std::construct_at` 在原始映射里建立 Shared 对象。这个技巧不意味着任意 C++ 类型都可安全跨进程使用；操作系统提供的 process-shared 同步也有自身初始化/销毁协议。

真实布局应明确版本、字段大小、偏移、长度、对齐、所有权和初始化完成标记。不同程序 ABI/架构之间还要定义编码，不只是“两个 sizeof 恰好相等”。

### 13.6.5 原书共享内存聊天室如何理解

原书为客户分配共享接收槽，子进程把收到的正文写进槽，用 Unix socketpair 通知父进程；父进程通知其他子进程读取对应位置再写到各客户端。

这是“数据面（共享正文）+ 控制面（通知槽位置）”组合。它减少正文来回经父进程复制的次数，但通知不自动保护槽：写者覆盖下一条消息前必须确保消费者已消费，或使用序号、确认、环形队列及同步。还需明确消息长度，不能一律发送固定缓冲区大小。

本目录拆成独立小实验，未照搬书中多进程聊天室。这样先学清同步、映射、资源回收，再尝试进程池/生产者消费者队列。

## 13.7 消息队列

**书中对照，PDF 第 494～499 页**：System V 消息由正 long 类型值与正文组成，接收时可以按类型选择，区别于纯 FIFO 字节流。msgget 创建/获取，msgsnd 发送，msgrcv 接收，msgctl 查询/删除。

```c
struct Message {
    long type;
    char text[128];
};
```

msgsnd/msgrcv 的正文长度 **不含开头的 long type**。不能直接传 `sizeof(Message)` 当正文长度，否则会访问结构尾部之外的字节。二进制正文也不要求 NUL 结尾，打印时使用实际收到的长度。

### 13.7.1 msgget

返回非负队列 ID，失败 -1。key、IPC_CREAT/EXCL、0600 权限等思想与 semget 类似。队列属于内核 IPC 对象，不是任意 close 可回收的 fd。

### 13.7.2 msgsnd

成功返回 0。队列满时阻塞；IPC_NOWAIT 让它以 EAGAIN 返回。中断可能 EINTR，队列删除可能 EIDRM。共享对象生命周期与发送者退出是不同事情：发送者结束后已排队消息仍可存在。

### 13.7.3 msgrcv

**正确返回值是 ssize_t，成功返回正文实际字节数**，可以为 0；失败 -1。原书写成 int 或“成功为 0”的段落应修正。

| msgtype | 选择规则 |
|---|---|
| 0 | 队列中最前消息 |
| 正数 | 首条匹配该 type 的消息（额外 flag 可改变规则） |
| 负数 | 类型值不大于其绝对值范围内，选择最低类型值对应的首条消息 |

因此不能把负值规则简化为“读第一条比绝对值小的消息”。若缓冲太小而未允许截断，可报 E2BIG，消息通常仍留队列；MSG_NOERROR 会允许截断并丢弃剩余正文。IPC_NOWAIT 没有匹配消息则 ENOMSG，和发送满队列的 EAGAIN 区分。[msgop(2)](https://man7.org/linux/man-pages/man2/msgop.2.html)

### 13.7.4 msgctl

IPC_STAT 查询，IPC_SET 设部分属性，IPC_RMID 删除队列。标识符是 msqid，不是共享内存 shmid。退出进程不自动删除它。

实验子进程发送一条 type=7 的小消息，父进程确认 child 成功后以 NOWAIT 收取并打印字节数，最后 IPC_RMID。这里只能这样先 wait，因为单条小消息能入队；若大量正文使队列满，父进程先等发送者、发送者又等父进程消费，就会互相等待。

现代项目也可选择 POSIX mq 或 Unix 域 SOCK_SEQPACKET/业务队列，按消息大小、背压、权限和事件循环支持选择。System V 队列不像 socket fd 那样可直接注册 epoll。

## 13.8 IPC 命令

**书中对照，PDF 第 500～501 页**：用 ipcs 观察共享内存、信号量、消息队列，说明多个 key=0 的对象来自 IPC_PRIVATE；ipcrm 清理遗留对象。

```bash
ipcs -m    # System V 共享内存
ipcs -s    # System V 信号量集合
ipcs -q    # System V 消息队列
```

检查对象 ID、owner、权限、容量、附加数量等；同一个 IPC namespace 中看得到的对象，不代表所有 namespace 的全局资源。

若你明确确认某个 **ID 属于自己创建且已无使用者的实验对象**，对应删除命令是：

```bash
ipcrm -m SHMID
ipcrm -s SEMID
ipcrm -q MSQID
```

这里 SHMID/SEMID/MSQID 是占位词，执行前替换。不要为清理一个实验对象删除整台机器的全部 IPC。POSIX shm 名称不属于 System V ipcs 管理范围，使用 shm_unlink；进程映射是否还存活要分别判断。

本目录 System V 示例输出自己的 ID，默认都会回收。若收到 SIGKILL 或机器异常中止，信号量/消息队列仍可能遗留；不要依赖 C++ 析构解决所有进程崩溃情形。

## 13.9 在进程间传递文件描述符

**书中对照，PDF 第 502～504 页**：通过 Unix 域 socket 的 sendmsg/recvmsg 传 SCM_RIGHTS，使接收进程得到指向同一内核文件对象的新 fd。原书示例用子进程打开文件再发给父进程。

### 13.9.1 为什么发整数不行

发送者的 fd=5 只在发送者 fd 表里有意义。接收者的 fd=5 可能是另一个 socket，或根本未打开。普通数据里传一个 5，不会让内核把资源映射给接收者。

SCM_RIGHTS 让内核保持所指对象引用，并为接收者安装一个 fd。新 fd 数值可能与原来相同，也可能不同，不能靠数字是否变化证明传递成功。它们引用同一 open file description，文件偏移和 O_NONBLOCK 等状态可共享。[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)

### 13.9.2 辅助数据的正确布局

```text
普通 payload（如一个 'F' 字节）
辅助区：cmsghdr { level=SOL_SOCKET, type=SCM_RIGHTS, length }
        后面是待传 fd 的 int 数据，含平台要求的对齐/填充
```

- 控制缓冲区用 `CMSG_SPACE(sizeof(int))` 分配，保证有 header、正文与对齐空间。
- header.cmsg_len 用 `CMSG_LEN(sizeof(int))`。
- CMSG_FIRSTHDR/CMSG_NXTHDR 找 header，CMSG_DATA 找正文。
- 用 memcpy 存取 fd 值，避免手工对齐指针假设。
- msghdr 全部初始化；检查 sendmsg/recvmsg 返回值。
- 接收检查 level/type/length，拒绝 MSG_CTRUNC/不符合协议的附加信息。
- Linux 可用 `MSG_CMSG_CLOEXEC` 原子设置收到 fd 的 close-on-exec。

原书代码清单中“仅声明一个 cmsghdr，然后向其后写 fd”以及零长度数组却传一字节的写法存在存储越界风险。不能把示意结构直接当可用控制缓冲区。

### 13.9.3 配套 pass_fd 做了什么

创建匿名化临时文件，写入内容并回到偏移 0；建立 Unix datagram socketpair，再 fork。子进程把文件 fd 通过 SCM_RIGHTS 入队并退出。父进程先关闭自己的原文件 fd，确认子进程发送成功后 recvmsg，检查收到的 fd 和 CLOEXEC，读取正文。

即使发送者已经退出，队列里携带的内核引用仍可让接收者获得文件访问能力。示例中的临时文件名字已 unlink，最终引用关闭后回收。

在接入进程向工作进程派发 accepted socket 时，也可采用这种方法。移交后谁负责 close、共享偏移是否允许、有没有额外副本，都要规定。传 fd 本身传递访问能力，所以真实服务应校验对端凭据、允许的对象类型和数量。

## 14. 现代 C++11～20 怎么组织多进程代码

| 标准 | 相关能力 | 不能误解为 |
|---|---|---|
| C++11 | RAII、移动语义、unique_ptr、error_code、atomic | 已有标准 fork 或跨进程 atomic 保证 |
| C++14 | std::exchange、make_unique 等 | 自动回收内核 IPC 对象 |
| C++17 | string_view、filesystem、optional、from_chars | 普通字符串/指针能直接共享给别的进程 |
| C++20 | span、construct_at、jthread、协程、counting_semaphore | 协程等于子进程、std semaphore 可直接跨进程共享 |

### 14.1 明确“哪一边拥有资源”

本目录 C++ `UniqueFd` 负责 close；Mapping 负责 munmap；NameOwner 负责 shm_unlink；SemaphoreOwner 负责 sem_destroy；Child 负责回收短子进程。这些是不同资源，不能只用一个 fd 析构涵盖。

fork 会让子进程继承普通内存里的所有者状态。若父子都走 C++ 析构，可能重复 unlink/删除同一个全局 IPC 对象。所以要明确父/子责任，或所有者记录 creator PID。实验子路径 `_exit`，全局对象清理由父进程负责。

RAII 应用在正常退出和异常展开，不保证 kill -9、崩溃或断电会执行。生产服务还需启动恢复、资源命名和外部管理策略。

### 14.2 多进程服务器设计

一客户一 fork 容易理解，但频繁创建、资源继承和回收有成本。prefork 进程池可预先启动有限 worker，接入层通过 fd/control channel 派发工作；每个 worker 仍可用 epoll 和协程管理多个连接。

如果 worker 使用线程池，应规划 fork 在启动早期完成，或用 spawn 启动独立 worker 可执行文件。共享内存/消息 IPC 可以隔离重计算或不可信组件，但超时、重启、取消、协议版本和部分结果必须设计。

可移植项目通常借助进程/IPC 库，例如 Boost 的相关组件；API 要按版本选择。Linux-specific 项目可直接封装 POSIX/内核接口。C++20 标准本身不提供完整子进程 API。

### 14.3 序列化与背压

pipe、socket、消息队列上的正文应该是协议，而不是直接传整个含指针的 C++ 对象。规定字段大小、编码和最大长度，先校验再分配。跨机器/不同 ABI 时尤其如此。

队列满、pipe 满、接收者很慢时必须控制上游工作量。不要让用户态队列无限增长，或在事件循环里阻塞等某个 child/IPC；将等待接到合适事件机制，并设置任务期限。

## 15. 原书中需要修正的几处关键表述

| 原文位置/主题 | 学习时采用的准确理解 |
|---|---|
| 13.1 信号“位图被清除” | 子进程 pending signals 初始为空；处理器和 mask 通常继承 |
| 13.3 父进程结束但子还运行 | 这是孤儿/被收养的运行进程，不自动是僵尸 |
| 13.5 原子操作不存在 | C++11 有 atomic；跨进程使用需确认 OS/实现约束 |
| 多处 ID 是正整数 | 0 也可能成功；失败值为 -1 |
| 13.7 msgrcv 成功返回 0 | 正确类型 ssize_t，成功返回实际正文长度 |
| 13.7 负 msgtype | 范围内选择最低类型值，含不大于边界的类型 |
| 13.9 CMSG 缓冲 | 分配 CMSG_SPACE 大小的真实缓冲，不只一个 header |

## 16. 运行与复习

完整命令和源码解析见 [README](../code/base_multi_process/readme.md)。本次实验只创建自己的临时文件/IPC、回收自己的 child，不部署服务器或更改系统 IPC 配置。

试着回答：为什么 fork 后 value 不共享但 fd 偏移可共享？exec 为什么不返回？为什么孤儿不等于僵尸？关闭 pipe 一份写端为什么未必 EOF？共享内存为什么还需要 semaphore？消息长度为什么不包括 type？为何不能通过发送整数实现 fd 传递？

相关接口可查：

- [fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)、[wait(2)](https://man7.org/linux/man-pages/man2/wait.2.html)。
- [posix_spawn(3)](https://man7.org/linux/man-pages/man3/posix_spawn.3.html)。
- [sysvipc(7)](https://man7.org/linux/man-pages/man7/sysvipc.7.html)。
- [semop(2)](https://www.man7.org/linux/man-pages/man2/semop.2.html)、[msgop(2)](https://man7.org/linux/man-pages/man2/msgop.2.html)。
- [shm_open(3)](https://man7.org/linux/man-pages/man3/shm_open.3.html)、[sem_init(3)](https://man7.org/linux/man-pages/man3/sem_init.3.html)。
- [unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)。

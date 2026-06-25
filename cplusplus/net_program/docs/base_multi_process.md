# 多进程服务器端编程 — 从 fork 到并发网络服务器

> **目标读者**：已掌握基础 socket 编程（socket / bind / listen / accept），希望深入理解多进程并发服务器的底层原理与实现。
> **阅读路径**：从操作系统进程模型开始 → fork 内核实现 → COW / 僵尸进程 / 信号处理 → 构建第一个并发服务器 → 进阶模式与陷阱。

---

## 目录

1. [进程：操作系统视角](#1-进程操作系统视角)
2. [`fork()` 系统调用：内核到底做了什么](#2-fork-系统调用内核到底做了什么)
3. [写时复制（Copy‑On‑Write, COW）](#3-写时复制copyonwrite-cow)
4. [进程生命周期与僵尸进程](#4-进程生命周期与僵尸进程)
5. [`wait()` / `waitpid()` 与 `SIGCHLD` 信号](#5-wait--waitpid-与-sigchld-信号)
6. [第一个多进程并发服务器](#6-第一个多进程并发服务器)
7. [深入：多进程 accept 的序列化问题](#7-深入多进程-accept-的序列化问题)
8. [进程池（Pre‑fork）模式](#8-进程池prefork模式)
9. [进程间通信（IPC）概要](#9-进程间通信ipc概要)
10. [多进程 vs 多线程 vs 协程](#10-多进程-vs-多线程-vs-协程)
11. [常见陷阱与最佳实践](#11-常见陷阱与最佳实践)
12. [附录：关键系统调用速查](#12-附录关键系统调用速查)

---

## 1. 进程：操作系统视角

### 1.1 什么是进程

进程是**资源分配的最小单位**，也是**内核调度**的基本实体。每个进程拥有独立的：

| 资源 | 说明 |
|------|------|
| **虚拟地址空间** | 代码段、数据段、堆、栈，完全隔离 |
| **页表** | 虚拟地址 → 物理地址的映射，**页表隔离是进程隔离的硬件基础** |
| **文件描述符表** | 打开的文件、socket、管道等 |
| **信号处理函数表** | 每个信号可注册独立 handler |
| **进程上下文** | 寄存器值、程序计数器（PC）、栈指针（SP） |
| **内核栈** | 系统调用陷入内核时使用 |
| **PID / TGID** | 进程 ID / 线程组 ID（Linux 中线程本质是轻量级进程） |

### 1.2 进程控制块（PCB）—— `task_struct`

内核中每个进程由一个巨大的结构体 `struct task_struct` 描述（Linux 源码 `include/linux/sched.h`），关键字段：

```c
// 简化示意 — 实际有数百个字段
struct task_struct {
    pid_t               pid;            // 进程 ID
    pid_t               tgid;           // 线程组 ID
    struct task_struct  *parent;        // 指向父进程
    struct list_head    children;       // 子进程链表
    struct list_head    sibling;        // 兄弟进程链表
    
    struct mm_struct    *mm;            // 内存描述符（页表、VMA）
    struct files_struct *files;         // 文件描述符表
    struct signal_struct *signal;       // 信号处理
    
    unsigned int        state;          // TASK_RUNNING / TASK_INTERRUPTIBLE / TASK_ZOMBIE …
    int                 exit_code;      // 退出码
    int                 exit_signal;    // 退出时发送给父进程的信号（默认 SIGCHLD）
    
    /* 调度相关 */
    struct sched_class  *sched_class;
    unsigned int        policy;
    int                 prio;
    
    /* 时间统计 */
    u64                 utime;          // 用户态时间
    u64                 stime;          // 内核态时间
};
```

> **关键理解**：`fork()` 的本质就是**深拷贝 `task_struct`**，然后修改少数字段（pid、parent、pptr 等）。

---

## 2. `fork()` 系统调用：内核到底做了什么

### 2.1 调用链

```
用户程序
   └─ fork()                (glibc 封装)
        └─ sys_clone()      (内核入口)
             └─ kernel_clone() / copy_process()  (核心)
                  ├─ dup_task_struct()      ← 拷贝 task_struct、内核栈
                  ├─ copy_semundo()
                  ├─ copy_files()           ← 拷贝文件描述符表
                  ├─ copy_fs()              ← 拷贝 fs 上下文
                  ├─ copy_sighand()         ← 拷贝信号处理函数
                  ├─ copy_mm()              ← **COW 关键 — 见下文**
                  ├─ copy_namespaces()
                  ├─ sched_fork()           ← 初始化调度字段
                  ├─ copy_thread()          ← 拷贝寄存器上下文、栈
                  └─ wake_up_new_task()     ← 将子进程放入就绪队列
```

### 2.2 `fork()` 精确语义

```c
pid_t pid = fork();
```

- **一次调用，两次返回**
- 父进程中返回子进程的 PID（> 0）
- 子进程中返回 **0**
- 返回 -1 表示失败（errno 见 `man 2 fork`）

### 2.3 `fork()` 后父子进程的异同

| 属性 | 子进程 | 说明 |
|------|--------|------|
| **PID** | 新分配 | 唯一标识 |
| **PPID** | 父进程 PID | 父进程崩溃后变为 1（init） |
| **虚拟地址空间** | 独立但内容相同 | COW 机制 — 见下一节 |
| **页表** | 新分配，页标记只读 | 触发写时复制 |
| **文件描述符** | 拷贝（共享偏移量） | `dup()` 语义 |
| **打开的文件偏移** | 与父进程共享 | 父子共用一个内核文件表项 |
| **信号处理函数** | 继承 | 用 `sigaction()` 设置的保留 |
| **未决信号** | 清空 | 子进程不继承未决信号 |
| **定时器 / 异步 IO** | 不继承 | |
| **记录锁（fcntl）** | 不继承 | 父子独立 |
| **内存锁（mlock）** | 不继承 | |
| **pending 信号掩码** | 继承 | |

### 2.4 内核视角的 fork 时序图

```
父进程                              内核                                    子进程
   │                                  │                                        │
   │  fork()                          │                                        │
   │─────────────────────────────────>│                                        │
   │                                  │  copy_process():                       │
   │                                  │    ├─ alloc_pid()         分配新 PID   │
   │                                  │    ├─ dup_task_struct()   拷贝 PCB     │
   │                                  │    ├─ copy_mm()           COW 设置    │
   │                                  │    ├─ copy_files()        拷贝 fd 表   │
   │                                  │    ├─ sched_fork()        初始化调度  │
   │                                  │    ├─ copy_thread()       设返回值为 0 │
   │                                  │    └─ wake_up_new_task()  ← 子进程就绪│
   │                                  │                                        │
   │  ← 返回子进程 PID (>0)            │                          (调度后)     │
   │                                  │                        ← 返回 0       │
   │                                  │                                        │
   │  注：父子进程的执行顺序是"不确定"的  ——  取决于调度器                        │
```

---

## 3. 写时复制（Copy‑On‑Write, COW）

### 3.1 为什么需要 COW

`fork()` 早期实现（`fork()` 诞生于 Unix V7/BSD）会**完整复制父进程的地址空间**。对于 4GiB 地址空间（即使只用了 200MiB Rss），这意味着：

- O(n) 的时间复杂度（n = 使用的物理页数）
- 即时翻倍的内存占用
- **绝大多数场景的优化观察**：`fork()` 后立即 `exec()` 新程序，复制的内容全部作废

### 3.2 COW 工作原理

现代 `fork()`（Linux 内核 2.4.0+ 默认）的 `copy_mm()` 流程：

```
父进程页表                      COW fork 后                        写操作触发
┌──────────────┐              ┌──────────────┐                   ┌──────────────┐
│ 页: P1       │              │ 页: P1       │                   │ 页: P1'      │
│ 权限: RW     │              │ 权限: RO ←!   │                   │ 权限: RW     │
│ 物理页: A    │     fork()   │ 物理页: A    │   父进程写 P1    │ 物理页: C ←新 │
└──────────────┘    ──────→  └──────────────┘   ───────────→   └──────────────┘
                               ┌──────────────┐                   ┌──────────────┐
                               │ 页: P1       │                   │ 页: P1       │
                               │ 权限: RO     │   子进程写 P1    │ 权限: RW     │
                               │ 物理页: A    │   ───────────→   │ 物理页: D ←新 │
                               └──────────────┘                   └──────────────┘
```

**三步流程**：

1. **fork() 时**：子进程获得一份新的页表，所有可写页面的页表项权限从 `RW` 改为 `RO`，并标记为 **写时复制**（通过页表项中的 `PAGE_COW` 标志位或 dirty bit 机制）。

2. **写触发（Page Fault）**：当任一进程写入某个页面时，MMU 检测到该页权限为只读且被标记为 COW → 触发 **缺页异常 #PF**（page fault handler）。

3. **内核处理**（`do_wp_page()` 路径）：
   ```
   缺页异常入口
     └─ handle_pte_fault()
          └─ do_wp_page(addr, vma, page)     ← wp = write-protect
               ├─ 分配新物理页
               ├─ 拷贝旧页内容 → 新页
               ├─ 修改触发进程的页表：新页 → RW
               ├─ 检查另一方页表是否还引用旧页：
               │   ├─ 是 → 旧页保留 RO（对方写时再触发）
               │   └─ 否 → 旧页改回 RW（只有自己用了）
               └─ 释放旧页引用计数
   ```

### 3.3 COW 的页粒度

COW 的拷贝单位是**物理页帧（通常 4KiB）**，不是整个地址空间。这意味着：

- 如果父子进程各自只修改 1 byte，代价是各复制 1 个物理页（4 KiB）
- 如果地址空间是 1 GiB RSS，但只写 10 页，代价是 10 × 4 KiB = 40 KiB + 页表拷贝
- **相比之下**，原始 fork 需要拷贝 1 GiB

### 3.4 哪些资源会被 COW？

| 资源 | COW 策略 |
|------|----------|
| **匿名页（堆、栈、BSS）** | ✅ COW |
| **文件映射页（mmap MAP_SHARED）** | ❌ 共享，不 COW |
| **文件映射页（mmap MAP_PRIVATE）** | ✅ COW |
| **内核数据结构（task_struct）** | ❌ 直接拷贝（很小） |
| **页表本身** | ❌ 直接拷贝（约 8–16 KiB 每进程） |
| **文件描述符表** | ❌ 直接拷贝（浅拷贝，共享内核文件表项） |

### 3.5 `vfork()` — COW 的极端优化

```c
pid_t vfork(void);
```

`vfork()` 甚至不拷贝页表，父子进程**共享同一地址空间**，父进程阻塞直到子进程 `exec()` 或 `_exit()`。用于 `fork()` + 立即 `exec()` 的场景优化。

**⚠️ 危险**：子进程修改变量将直接影响父进程。现代 Linux 中 `vfork()` 实现已通过 COW 变得不那么必要，但某些嵌入式 / 性能敏感场景仍有使用。

---

## 4. 进程生命周期与僵尸进程

### 4.1 六种进程状态（Linux）

```c
// include/linux/sched.h
#define TASK_RUNNING            0x0000  // 就绪或运行中
#define TASK_INTERRUPTIBLE      0x0001  // 可中断睡眠（等待 IO / 信号）
#define TASK_UNINTERRUPTIBLE    0x0002  // 不可中断睡眠（等待内核资源）
#define __TASK_STOPPED          0x0004  // 收到 SIGSTOP / SIGTSTP
#define __TASK_TRACED           0x0008  // 被 ptrace 跟踪
#define EXIT_DEAD               0x0010  // 真正的死亡（已从 task 链表删除）
#define EXIT_ZOMBIE             0x0020  // 僵尸状态
```

### 4.2 僵尸进程（Zombie）—— 内核视角

**定义**：子进程已退出但父进程尚未调用 `wait()` / `waitpid()` 回收其 PCB 时的状态。

**内核中发生了什么**：

```
子进程调用 exit() / return
  └─ do_exit()
       ├─ 释放用户态资源（mm、files、fs 等）
       ├─ 释放内核栈（部分）
       ├─ 向父进程发送 SIGCHLD 信号
       ├─ 将自身状态设为 EXIT_ZOMBIE
       └─ PCB (task_struct) 保留 —— 只保留极小信息：
           ├─ pid
           ├─ exit_code          ← wait() 读取此值
           ├─ utime / stime      ← 进程统计
           └─ 信号通知信息
```

**僵尸进程的特点**：
- 不占用 CPU
- 不占用内存（仅保留极小 PCB，约 1-2 KiB）
- **不释放 PID**（PID 是有限的资源，内核 `pid_max` 默认 32768）
- 在 `ps` 中显示为 `<defunct>` 或 `Z`

### 4.3 僵尸进程的危害

```bash
$ cat /proc/sys/kernel/pid_max
4194304 # 32768是之前的
```

如果父进程不 `wait()`，每来一个客户端就产生一个僵尸，反复创建子进程将**耗尽 PID 空间**，导致 `fork()` 返回 -1（EAGAIN）。

```
ps aux | grep Z
USER       PID  STAT  COMMAND
user      1234  Z     [server] <defunct>   ← 僵尸
user      1235  Z     [server] <defunct>   ← 堆积
```

### 4.4 孤儿进程（Orphan）

**定义**：父进程先于子进程退出时，子进程成为孤儿。

**内核处理**：孤儿进程会被 **init 进程（PID 1）收养**，由 init 负责在其退出时 `wait()` 回收。

```
父进程崩溃 → exit(2)
  └─ kernel/exit.c: forget_original_parent()
       ├─ 遍历 children 链表
       └─ re-parent 到 PID 1（或当前线程组的 init）
```

**实际影响**：
- 孤儿进程不会变成僵尸（init 会自动 wait）
- 子进程仍在后台运行，直到自己退出
- 常用于 **daemon 化**：父进程 fork 后立即退出，子进程脱离终端控制

---

## 5. `wait()` / `waitpid()` 与 `SIGCHLD` 信号

### 5.1 基础回收：`wait()`

```c
#include <sys/wait.h>

pid_t wait(int *status);
// 返回：已回收的子进程 PID，-1 表示无子进程
// status：输出参数，编码子进程退出信息
```

**阻塞行为**：
- 如果有子进程已僵尸 → 立即回收并返回
- 如果没有子进程已退出但还有子进程存活 → **阻塞**
- 调用进程没有子进程 → 返回 -1（ECHILD）

### 5.2 `waitpid()` — 精细控制

```c
pid_t waitpid(pid_t pid, int *status, int options);
```

| `pid` 参数 | 含义 |
|------------|------|
| `> 0` | 等待指定 PID 的子进程 |
| `= -1` | 等待任意子进程（等价于 `wait()`） |
| `= 0` | 等待与调用者同进程组的任意子进程 |
| `< -1` | 等待进程组 ID 等于 `\|pid\|` 的任意子进程 |

| `options` 标志 | 含义 |
|----------------|------|
| `WNOHANG` | **非阻塞** — 无已退出子进程时立即返回 0 |
| `WUNTRACED` | 也返回被停止的子进程状态 |
| `WCONTINUED` | 也返回被 SIGCONT 恢复的子进程状态 |

### 5.3 解析子进程退出状态 — `status` 的位编码与宏原理

#### 5.3.1 一个 int 承载四种信息

`wait()` / `waitpid()` 的 `status` 参数是一个 **int（32位）**，内核用它的不同比特区间编码四种不同的退出场景：

```
status (int) 的位布局（Linux x86 / ARM，POSIX 标准）：

位 31 ...  16 | 15 ... 8 | 7 ... 0
─────────────────────────────────────
    (未使用)     退出码      终止信号
                (exit_code)  (term_sig)
```

但这不是故事的全部 — 编码方式随**退出原因**而不同：

| 退出场景 | 位 7–0（低 8 位） | 位 15–8（次低 8 位） | 其他位 |
|----------|-------------------|----------------------|--------|
| **正常退出** (`exit()` / `return`) | 0 | 退出码 (0–255) | 0 |
| **被信号杀死** | 信号编号 | 0（若 core dump 则置特定位） | — |
| **被信号停止**（如 SIGSTOP） | 信号编号 | 0x7F (`__W_STOPCODE`) | — |
| **被 SIGCONT 恢复** | SIGCONT | 0x7F (`__W_STOPCODE`) | — |

#### 5.3.2 宏展开 — 位运算原理

所有宏定义位于 `/usr/include/sys/wait.h`（或 `/usr/include/bits/waitstatus.h`）：

```c
/* 核心常量 */
#define __W_EXITCODE(ret, sig)  ((ret) << 8 | (sig))      // 内核构造 status 的公式
#define __W_STOPCODE(sig)        ((sig) << 8 | 0x7F)       // 停止状态的编码
#define __W_CONTINUED            0xFFFF                     // 继续状态的魔数

/* ─── 判断类宏 ─── */
#define WIFEXITED(status)        (__WTERMSIG(status) == 0)  // 终止信号为 0 → 正常退出
#define WIFSIGNALED(status)      (__WTERMSIG(status) != 0 && \
                                   __WCOREDUMP(status) == 0) // 有终止信号且非 stop/cont
#define WIFSTOPPED(status)       ((status) & 0xFF) == 0x7F   // 低 8 位 == 0x7F
#define WIFCONTINUED(status)     ((status) == __W_CONTINUED) // status == 0xFFFF

/* ─── 取值类宏 ─── */
#define WEXITSTATUS(status)      (((status) >> 8) & 0xFF)   // 取次低 8 位 → 退出码
#define WTERMSIG(status)         ((status) & 0x7F)           // 取低 7 位 → 信号编号
#define WCOREDUMP(status)        ((status) & 0x80)           // 第 7 位 → core dump 标志
#define WSTOPSIG(status)         WEXITSTATUS(status)         // 停止场景下次低 8 位即信号编号

/* 辅助宏（内部）*/
#define __WTERMSIG(status)       ((status) & 0x7F)
#define __WCOREDUMP(status)      ((status) & 0x80)
```

**关键区别**：

| 状态 | (status & 0x7F) | 含义 |
|------|-----------------|------|
| 0x00 | 0 | 正常退出（`WIFEXITED`） |
| 0x01–0x1E | 1–30 | 被信号终止（`WIFSIGNALED`，信号编号即为该值） |
| 0x7F | 127 | 被信号暂停（`WIFSTOPPED`，信号编号来自次低 8 位） |
| 0xFFFF | — | `WIFCONTINUED`（整体匹配魔数） |

#### 5.3.4 内核视角：从 `do_exit()` 到用户态 `status`

```
子进程调用 exit(42)
  │
  ▼
do_exit(42)                       ← kernel/exit.c
  ├─ 检查是否有还在运行的线程
  ├─ exit_signals()               ← 发送 SIGCHLD
  ├─ exit_mm()                    ← 释放地址空间
  ├─ exit_files()                 ← 释放文件描述符
  ├─ exit_fs()                    ← 释放 fs 上下文
  ├─ ...
  ├─ ts->exit_code = 42           ← 存入 task_struct
  │   后面被 encode 成 __W_EXITCODE(42,0) = 0x2A00
  ├─ ts->state = EXIT_ZOMBIE      ← 变成僵尸
  ├─ 向父进程发送 SIGCHLD
  └─ schedule()                   ← 让出 CPU

父进程调用 wait(&status)
  │
  ▼
sys_wait4()                       ← kernel/exit.c
  ├─ find_get_task()              ← 查找一个 EXIT_ZOMBIE 子进程
  ├─ put_task() / release_task():
  │    ├─ 将 ts->exit_code 编码到 status 的位布局
  │    ├─ 拷贝到用户态: copy_to_user(status, &encoded_status, sizeof(int))
  │    ├─ 释放 PCB: kmem_cache_free(task_struct_cachep, ts)
  │    └─ 回收 PID: free_pid(ts->pid)
  └─ 返回子进程 PID

用户态程序:
    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);   // 位运算取出 → 42
    }
```

#### 5.3.5 完整使用示例

```c
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程 */
        printf("Child PID=%d, will exit(42)\n", getpid());
        exit(42);
    }

    /* 父进程 */
    int status;
    pid_t ret = waitpid(pid, &status, 0);  // 阻塞等待指定子进程
    if (ret == -1) {
        perror("waitpid");
        return 1;
    }

    printf("Reaped child PID=%d\n", ret);

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        printf("Normal exit, code=%d\n", exit_code);         // 42
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        int core = WCOREDUMP(status);
        printf("Killed by signal %d%s\n", sig,
               core ? " (core dumped)" : "");
    }

    if (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);
        printf("Stopped by signal %d\n", sig);
    }

    if (WIFCONTINUED(status)) {
        printf("Resumed by SIGCONT\n");
    }

    return 0;
}
```

编译运行：

```bash
$ gcc -o wexit_test wexit_test.c && ./wexit_test
Child PID=12345, will exit(42)
Reaped child PID=12345
Normal exit, code=42
```

#### 5.3.6 各宏的详细行为总结

| 宏 | 返回类型 | 含义 | 底层位操作 |
|----|---------|------|-----------|
| `WIFEXITED(status)` | `int` (bool) | 子进程是否**正常退出**（调用了 `exit()` / `_exit()` / `return`） | `(status & 0x7F) == 0` |
| `WEXITSTATUS(status)` | `int` | 正常退出的退出码（0–255） | `(status >> 8) & 0xFF` |
| `WIFSIGNALED(status)` | `int` (bool) | 子进程是否被**信号杀死**（非 stop/cont） | 见上面逻辑 |
| `WTERMSIG(status)` | `int` | 杀死子进程的**信号编号**（1–31） | `status & 0x7F` |
| `WCOREDUMP(status)` | `int` (bool) | 是否产生了 **core dump** 文件 | `status & 0x80` |
| `WIFSTOPPED(status)` | `int` (bool) | 子进程是否因信号**暂停**（需 `WUNTRACED`） | `(status & 0xFF) == 0x7F` |
| `WSTOPSIG(status)` | `int` | 导致暂停的**信号编号** | `(status >> 8) & 0xFF`（同 `WEXITSTATUS`） |
| `WIFCONTINUED(status)` | `int` (bool) | 子进程是否被 **SIGCONT 恢复**（需 `WCONTINUED`） | `status == 0xFFFF` |

#### 5.3.7 架构差异与注意事项

1. **位数差异**：以上位布局是 Linux x86/x86_64/ARM。其他 Unix（如 FreeBSD、macOS）保持一致，但 POSIX 不强制具体位布局——**宏是唯一的可移植接口**。

2. **退出码截断**：`exit(n)` 的参数 n 在 Linux 上会被截断为 **低 8 位**（0–255）：
   ```c
   exit(256);   // WEXITSTATUS → 0
   exit(300);   // WEXITSTATUS → 44 (300 & 0xFF)
   exit(-1);    // WEXITSTATUS → 255 ((-1) & 0xFF)
   ```

3. **`WCOREDUMP` 非 POSIX 标准**：它是 Linux/BSD 扩展，不是所有 Unix 系统都提供。

4. **不要直接操作 `status` 位**：始终使用宏。代码如 `status >> 8` 会在不支持该位布局的系统上出错。

5. **信号编号范围**：`WTERMSIG` 取低 7 位（0–127），Linux 标准信号为 1–31，POSIX 实时信号 32–64，实时信号范围正好在 7 位之内。如需区分实时信号编号，必须使用宏。

#### 5.3.8 实战指南 — 各宏的最佳使用时机

```c
/* ─── 模式 1：区分正常退出 vs 信号杀死 ─── */
if (WIFEXITED(status)) {
    // 子进程主动退出，用 WEXITSTATUS 获取返回值
    // 场景：子进程处理完客户端 exit(0)，或发生业务错误 exit(1)
    int code = WEXITSTATUS(status);
    if (code != 0) log("child exited with error %d", code);
} else if (WIFSIGNALED(status)) {
    // 子进程被信号杀死，用 WTERMSIG + WCOREDUMP
    // 场景：SIGSEGV(段错误)、SIGABRT(assert失败)、SIGKILL(被kill -9)
    int sig = WTERMSIG(status);
    log("child killed by signal %d%s", sig,
        WCOREDUMP(status) ? " (core)" : "");
}

/* ─── 模式 2：结合非阻塞回收 ─── */
while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    if (WIFEXITED(status)) {
        // 正常退出 — 最理想
    } else if (WIFSIGNALED(status)) {
        // 异常终止 — 可能需要重启 worker
        spawn_new_worker();
    }
    // WIFSTOPPED/WIFCONTINUED 在网络服务器中通常不关心
}

/* ─── 模式 3：gdb 调试时手动查看 status ─── */
// (gdb) p/d status          ← 十进制查看
// (gdb) p/t status          ← 二进制查看
// (gdb) p WEXITSTATUS(status)  ← 直接用宏
```

#### 5.3.9 `exit_code` 在内核中的完整流转路径

```
用户态调用                        内核态
─────────────────                ─────────────────
exit(42)  →  sys_exit()
              └─ do_exit(42)
                   ├─ ts->exit_code = 42
                   ├─ … 释放资源 …
                   └─ ts->state = EXIT_ZOMBIE

                                     (父进程调用 wait)
                                              ↓
sys_wait4()
  └─ wait_consider_task(ts)
       ├─ encoded = __W_EXITCODE(ts->exit_code, ts->signal->group_exit_code)
       │         // 对于正常退出: (42 << 8) | 0 = 0x2A00
       │         // 对于信号:     (0 << 8) | sig = 0x0009 (if SIGKILL)
       ├─ put_user(encoded, &p->status)   // 写回用户态
       └─ release_task(ts)
            ├─ __exit_signal(ts)         // 从全局 pid 链表移除
            ├─ __exit_sighand(ts)        // 释放信号处理
            ├──__unhash_process(ts)
            └─ free_task(ts)             // 真正释放 task_struct
```

此流程解释了为什么**必须在子进程退出后尽快 `wait()`**——僵尸的 `task_struct` 虽小，但 `pid` 资源被占用，且 `task_struct` 的 slab 缓存也可能被耗尽（详见 [4.3 僵尸进程危害](#43-僵尸进程的危害)）。

---

### 5.4 `SIGCHLD` 信号 — 异步回收

**问题**：`wait()` 阻塞，`waitpid(WNOHANG)` 需要轮询。如何让子进程退出时自动触发回收？

**答案**：注册 `SIGCHLD` 信号处理函数。

```c
void sigchld_handler(int sig) {
    // 注意：信号处理函数中只能调用 async-signal-safe 的函数
    int saved_errno = errno;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        // 回收了一个子进程
    }
    errno = saved_errno;
}

// 注册
struct sigaction sa = {0};
sa.sa_handler = sigchld_handler;
sigemptyset(&sa.sa_mask);          // 不阻塞额外信号（已被 SA_NOCLDSTOP 覆盖）
sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;  // SA_NOCLDSTOP: 只对退出通知感兴趣
sigaction(SIGCHLD, &sa, NULL);
```

**⚠️ 关键细节**：
1. **`while` 循环**：多个子进程同时退出时，一次信号可能对应多个退出 — 必须循环 `waitpid` 直到返回 0
2. **`SA_RESTART`**：被信号中断的系统调用（如 `accept`）自动重启，否则必须处理 `EINTR`
3. **`SA_NOCLDSTOP`**：子进程收到 `SIGSTOP`/`SIGCONT` 时不触发 handler
4. **`async-signal-safe`** 限制：信号处理函数中只能调用 write、wait、exit 等少数函数（不可调用 printf、malloc 等）

### 5.5 信号驱动 vs 阻塞 wait 的权衡

| 方式 | 优点 | 缺点 |
|------|------|------|
| `wait()` 阻塞 | 逻辑简单 | 父进程无法做其他事 |
| `waitpid(WNOHANG)` 轮询 | 非阻塞 | 浪费 CPU（忙等） |
| `SIGCHLD` 信号 | 异步，高效 | 信号处理复杂性 |
| `sigwaitinfo()` | 同步等待信号 | 需要子进程创建时额外步骤 |

**网络服务器推荐**：`SIGCHLD` 信号 + `while waitpid(WNOHANG)`，这是最通用的方案。

---

## 6. 第一个多进程并发服务器

### 6.1 最小原型：迭代服务器

先回顾**单进程迭代服务器**（一次只服务一个客户端）：

```c
// server_single.c
int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    bind(listen_fd, ...);
    listen(listen_fd, SOMAXCONN);

    while (1) {
        int conn_fd = accept(listen_fd, NULL, NULL);   // 阻塞
        // 处理客户端请求（阻塞期间不能接受新连接）
        serve_client(conn_fd);
        close(conn_fd);
    }
}
```

**问题**：`serve_client()` 执行期间无法接受新连接。如果每个请求耗时 100ms，QPS 被限制在 10。

### 6.2 多进程并发版本

```c
// server_mp_basic.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

/* 子进程处理客户端 */
void serve_client(int conn_fd) {
    char buf[1024];
    ssize_t n;
    while ((n = read(conn_fd, buf, sizeof(buf))) > 0) {
        write(conn_fd, buf, n);         // echo 服务
    }
    close(conn_fd);
    exit(0);                            // 子进程退出
}

/* SIGCHLD 处理 — 避免僵尸 */
void sigchld_handler(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // 回收已退出的子进程
    }
    errno = saved_errno;
}

int main() {
    /* 注册 SIGCHLD */
    struct sigaction sa = {0};
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    /* 创建监听 socket */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, SOMAXCONN);

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int conn_fd = accept(listen_fd,
                             (struct sockaddr*)&client_addr,
                             &addr_len);
        if (conn_fd < 0) {
            if (errno == EINTR) continue;   // 信号中断，重试
            perror("accept");
            break;
        }

        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            close(listen_fd);               // 子进程不需要监听 fd
            serve_client(conn_fd);
            /* 不会到达这里 */
        } else if (pid > 0) {
            /* 父进程 */
            close(conn_fd);                 // 父进程不需要连接 fd
        } else {
            perror("fork");
            close(conn_fd);
        }
    }

    close(listen_fd);
    return 0;
}
```

### 6.3 代码背后的内核视角

```
accept() → 内核从 SYN 队列取出已完成连接 → 返回新 fd
fork()   → 父子进程共享同一个 conn_fd 内核文件表项
            ┌──────────────────────┐
            │  内核文件表 (struct file)    │
            │  f_count = 2              │  ← 父子各引用一次
            │  f_pos (不适用 socket)     │
            │  sock->sk               │  ← 指向同一个 TCP socket
            │  ...                    │
            └──────────────────────┘
close(listen_fd) → 父进程将 listen_fd 引用计数 -1
close(conn_fd)   → 父进程将 conn_fd 引用计数 -1
                   子进程 close() 时计数归零 → 内核真正关闭连接
```

**关键**：文件描述符的 `close()` 只是减少引用计数，只有在计数为 0 时才真正关闭底层对象。因此父子进程各 `close()` 一次才是安全的。

### 6.4 为什么父子进程都要 close 各自的 fd

| 进程 | 操作 | 效果 |
|------|------|------|
| **父进程** | `close(conn_fd)` | 父进程不再持有连接，但子进程引用仍在，TCP 连接保持 |
| **子进程** | `close(conn_fd)` → `exit()` | 子进程文件表引用归零 → 内核发送 FIN，TCP 连接关闭 |
| **子进程** | `close(listen_fd)` | 子进程不再持有监听 socket，但父进程引用仍在 |
| **父进程** | `close(listen_fd)` → `exit()` | 内核真正关闭监听端口 |

> **不 close 的后果**：打开的文件描述符会泄漏，进程可打开的 fd 数量受 `ulimit -n` 限制（默认 1024）。

---

## 7. 深入：多进程 accept 的序列化问题

### 7.1 惊群效应（Thundering Herd）

传统 `accept()` 实现：多个进程 / 线程同时阻塞在 `accept()` 上，一个连接到来时**所有等待者被唤醒**，但只有一个能成功获取连接，其余重新进入休眠。

```
        ┌─── 进程 A 阻塞在 accept ────┐
客户端 → │   进程 B 阻塞在 accept       │ → 内核唤醒所有进程
  连接    │   进程 C 阻塞在 accept       │
        └─────────────────────────────┘
               ↓
        进程 A 成功 accept
        进程 B accept() → 返回 EAGAIN   ← 浪费一次上下文切换
        进程 C accept() → 返回 EAGAIN   ← 浪费一次上下文切换
```

### 7.2 现代内核修复：`SO_REUSEPORT` + `EPOLLEXCLUSIVE`

要理解这两个方案，首先要区分清楚它们解决的是**两种不同的架构场景**：

| 场景 | 共享的资源 | 问题 |
|------|-----------|------|
| **经典 Pre‑fork**（[第 8 节](#8-进程池prefork模式)） | 所有 worker 共享**同一个** `listen_fd` | 多个进程同时 `accept()` → 惊群 |
| **Multi‑socket Pre‑fork** | 每个 worker 有**自己独立**的 `listen_fd`，绑定同端口 | 无需惊群，但需要内核帮忙分发 |

---

#### 7.2.1 方案一：`SO_REUSEPORT` — 每个进程绑定自己的 socket

##### 解决什么问题

让多个进程可以**各自独立地创建 socket、绑定到同一个 IP:端口**。新连接到达时，内核直接把它放入**某个 socket 的 accept 队列**，其余进程根本不知道有这个连接。

```
传统 Pre‑fork（共享 listen_fd）:
                    ┌──────────────────────┐
                    │     listen_fd        │  ← 所有进程共享
                    │  内核 accept 队列     │
                    └──────────┬───────────┘
                               │ 新连接到达
                    ┌──────────┼──────────┐
                    ▼          ▼          ▼
                 worker A   worker B   worker C
                 accept()   accept()   accept()
                 ↑ 全被唤醒，只有一个成功 — 其他白跑一趟


SO_REUSEPORT（各自独立 socket）:
    进程 A: socket A ── bind(8080) ── 内核分发 ── 连接 1, 4, 7 …
    进程 B: socket B ── bind(8080) ── 内核分发 ── 连接 2, 5, 8 …
    进程 C: socket C ── bind(8080) ── 内核分发 ── 连接 3, 6, 9 …
                    ↑
              内核根据 (src_ip, src_port, dst_ip, dst_port) 四元组
              做 hash，将新连接放入对应 socket 的队列
              只有被选中的 socket 收到通知，另外两个**完全不知道**
```

##### 关键区别：和 `SO_REUSEADDR` 不是一回事

```
SO_REUSEADDR：允许 bind() 到 TIME_WAIT 状态的地址
              → 解决的是"服务器重启时 bind 失败"的问题
              → 不允许多个 socket 绑定到同一个端口

SO_REUSEPORT：允许多个 socket 精确绑定到同一 IP:端口
              → 解决的是"多进程分发"的问题
              → 内核做负载均衡
```

##### Pre‑fork + `SO_REUSEPORT` 完整示例

```c
// server_reuseport.c
// 编译: gcc -o server_reuseport server_reuseport.c
// 测试: 在另一个终端用 nc/telnet 连接

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define N_WORKERS 4

/* 每个 worker 独立创建并绑定自己的 socket */
int make_reuseport_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    // ⚠️ 注意：这里用的是 SO_REUSEPORT，不是 SO_REUSEADDR

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    listen(fd, SOMAXCONN);
    return fd;
}

void serve_client(int conn_fd) {
    char buf[1024];
    ssize_t n;
    while ((n = read(conn_fd, buf, sizeof(buf))) > 0) {
        write(conn_fd, buf, n);
    }
    close(conn_fd);
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    /* 注册 SIGCHLD */
    struct sigaction sa = {0};
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    /* 忽略 SIGPIPE — 防止 write 到关闭的连接时子进程被杀死 */
    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < N_WORKERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* ===== 子进程（worker）===== */
            /* ★ 关键：每个 worker 自己创建 socket、绑定同一端口 */
            int fd = make_reuseport_socket(PORT);

            printf("Worker PID=%d listening on port %d "
                   "(socket fd=%d)\n", getpid(), PORT, fd);

            while (1) {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int conn = accept(fd, (struct sockaddr*)&client, &len);
                if (conn < 0) {
                    if (errno == EINTR) continue;
                    break;
                }
                serve_client(conn);
            }
            close(fd);
            exit(0);
        }
        /* else: 父进程继续创建下一个 worker */
    }

    /* ===== 父进程（master）: 只负责监控 worker ===== */
    printf("Master PID=%d spawned %d workers\n", getpid(), N_WORKERS);
    while (1) {
        int status;
        pid_t dead = waitpid(-1, &status, 0);
        if (dead > 0) {
            printf("Worker %d exited, restarting...\n", dead);
            /* 重启：重新 fork */
            pid_t new = fork();
            if (new == 0) {
                int fd = make_reuseport_socket(PORT);
                while (1) {
                    int conn = accept(fd, NULL, NULL);
                    if (conn < 0) continue;
                    serve_client(conn);
                }
            }
        }
    }
}
```

##### 内核分发机制深入

`SO_REUSEPORT` 的内核实现位于 `net/ipv4/inet_hashtables.c`：

```
新 TCP 连接到达（SYN → SYN+ACK → ACK 完成三次握手）
  │
  ▼
__inet_hash_connect()
  └─ compute_score()       ← 匹配 port
       └─ 遍历所有绑定了该端口的 socket 链表
            │
            ├─ SO_REUSEPORT 组的 socket 被放入一个数组
            └─ 用四元组哈希选取目标 socket:
                 idx = hash_32(src_ip, src_port, dst_ip, dst_port) % group_size
                 selected_sock = group[idx]
                      │
                      ▼
                 selected_sock 的 accept 队列 ← 新连接
                 只有 selected_sock 被唤醒 → 完全无惊群
```

**哈希分发的特性**：
- 同一个客户端（四元组相同）总是被分到同一个 worker → **天然支持连接亲和性**
- 新增/减少 worker 时，哈希映射会变化 → 已有连接不受影响（已在 accept 队列中的不会移动）
- 分发粒度是**连接级别**，不是数据包级别

---

#### 7.2.2 方案二：`EPOLLEXCLUSIVE` — 共享 fd 只唤醒一个

##### 这个方案解决什么场景

很多现实服务器（如 Nginx）使用 **Pre‑fork + epoll** 架构：
- master 进程创建 `listen_fd`
- fork 出 N 个 worker
- **每个 worker 都把同一个 `listen_fd` 加入自己的 epoll 实例**
- 新连接来临时，内核需要决定**唤醒哪个 epoll 实例**

```
       ┌────────────── listen_fd ──────────────┐
       │         (所有 worker 共享)              │
       └────┬──────────┬──────────┬────────────┘
            │          │          │
        epoll A     epoll B     epoll C
            │          │          │
        worker A    worker B    worker C

新连接 → listen_fd 可读 → 内核检查 waiters 链表
                      → 默认行为：唤醒所有等待的 epoll 实例
                      → 问题：三个 epoll_wait 都返回，都去 accept，
                        但只有一个成功
```

**`EPOLLEXCLUSIVE` 做的事情**：当把 listen_fd 加入 epoll 时，加上这个标志，内核**只唤醒一个 epoll 实例**。

##### Pre‑fork + epoll + EPOLLEXCLUSIVE 完整示例

```c
// server_epoll_exclusive.c
// 编译: gcc -o server_epoll server_epoll_exclusive.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define N_WORKERS 4
#define MAX_EVENTS 16

int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(fd, SOMAXCONN);
    return fd;
}

void worker_loop(int listen_fd) {
    /* 每个 worker 创建自己的 epoll 实例 */
    int epfd = epoll_create1(0);

    struct epoll_event ev = {
        /* ★ EPOLLEXCLUSIVE: 多个 epoll 共享同一 fd 时只唤醒一个 */
        .events = EPOLLIN | EPOLLEXCLUSIVE,
        .data.fd = listen_fd,
    };
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    struct epoll_event events[MAX_EVENTS];

    printf("Worker %d ready\n", getpid());

    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                /* 有新连接 — 用 while 循环 accept 完所有已就绪的连接 */
                while (1) {
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int conn = accept(listen_fd,
                                      (struct sockaddr*)&client, &len);
                    if (conn < 0) {
                        if (errno == EAGAIN) break;  // 没更多连接了
                        if (errno == EINTR) continue;
                        break;
                    }
                    /* 处理连接（此处简化为 echo，实际应加入 epoll） */
                    serve_client(conn);
                }
            }
        }
    }
}

void serve_client(int conn_fd) {
    char buf[1024];
    ssize_t n;
    while ((n = read(conn_fd, buf, sizeof(buf))) > 0) {
        write(conn_fd, buf, n);
    }
    close(conn_fd);
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);  // 简化处理

    /* master 创建唯一的 listen_fd */
    int listen_fd = make_listen_socket(PORT);

    for (int i = 0; i < N_WORKERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            worker_loop(listen_fd);
            exit(0);
        }
    }

    printf("Master PID=%d, workers=%d\n", getpid(), N_WORKERS);
    while (1) pause();
}
```

##### EPOLLEXCLUSIVE 的内核行为对比

| 场景 | 无 EPOLLEXCLUSIVE | 有 EPOLLEXCLUSIVE |
|------|------------------|-------------------|
| listen_fd 可读时 | 遍历 waiters 链表，**全部**唤醒 | 只唤醒 waiters 链表的**头节点** |
| epoll_wait 返回 | 3 个 worker 都返回 → 2 个 accept 失败 | 仅 1 个 worker 返回 → 100% 成功 |
| 额外上下文切换 | 2 次（唤醒 + 发现 EAGAIN 后再休眠） | 0 次 |
| 负载均衡 | 无保证（取决于调度时机） | 近似轮转（链表的头节点不停变化） |

##### 内核 waiters 链表的变化

```
初始状态: 3 个 worker 都把 listen_fd 加入自己的 epoll
          ┌─── waiters 链表 ───┐
          │ epoll_A → epoll_B → epoll_C │
          └────────────────────────────┘

新连接到达:
  无 EPOLLEXCLUSIVE:
      遍历链表，唤醒 epoll_A、epoll_B、epoll_C
      → 3 个 worker 都从 epoll_wait 返回

  有 EPOLLEXCLUSIVE:
      只唤醒链表头的 epoll_A
      → epoll_A 被移到链表尾（公平性）
      → 只有 worker A 从 epoll_wait 返回
      → 下次连接唤醒 epoll_B，再下次 epoll_C（近似轮转）
```

---

#### 7.2.3 两个方案的对比与选择

| 维度 | `SO_REUSEPORT` | `EPOLLEXCLUSIVE` |
|------|----------------|------------------|
| **Linux 内核版本** | 3.9+ | 4.5+ |
| **架构** | 每个进程**独立 socket**，bind 同端口 | 所有进程**共享同一个 fd**，加入各自 epoll |
| **惊群消除** | ✅ 完全消除（内核分发） | ✅ 完全消除（仅唤醒一个） |
| **负载均衡** | 内核做 hash（四元组）—— 均衡性好 | 近似轮转（内核链表遍历），均衡性稍差 |
| **连接亲和性** | ✅ 同客户端 → 同 worker（hash 稳定） | ❌ 无保证（取决于调度时机） |
| **配置变更** | 需创建/销毁 socket | 只需增减 epoll 成员 |
| **非 epoll 场景** | ✅ 原生 accept 即可 | ❌ 必须使用 epoll |
| **代码复杂度** | 低（每个 worker 独立创建 socket） | 中（需要 epoll 事件循环） |
| **适用场景** | 新项目、希望简单稳定的预分叉 | 已有 epoll 架构的共享 fd 场景（如 Nginx） |

**简单选择原则**：

```
你的服务器是否已经在用 epoll？
  ├─ 是 → listen_fd 已由 master 创建，所有 worker 共享
  │     └─ 选择 EPOLLEXCLUSIVE（改动最小）
  │
  └─ 否（直接用 accept 阻塞 / 新建项目）
        ├─ 内核 ≥ 3.9 → 选择 SO_REUSEPORT（架构更干净）
        └─ 内核 < 3.9 → 只能忍受惊群，或用进程级锁序列化 accept
```

> **补充说明**：`SO_REUSEPORT` 和 `EPOLLEXCLUSIVE` 可以**组合使用**：每个进程创建自己的 socket（`SO_REUSEPORT`），然后把这个 socket 加入 epoll 并设置 `EPOLLEXCLUSIVE`，防止同一 socket 被多个 epoll 实例重复监听时的惊群。这种组合在 `nginx 1.9.1+` 的 `reuseport` 模式中实际使用。

### 7.3 accept 的 EINTR 处理

```c
accept_fd:
    conn_fd = accept(listen_fd, ...);
    if (conn_fd < 0) {
        if (errno == EINTR)  goto accept_fd;   // 信号中断，重试
        if (errno == ECONNABORTED) goto accept_fd;  // 客户端中途放弃
        if (errno == EMFILE) {                // 进程 fd 耗尽
            sleep(1);
            goto accept_fd;
        }
        perror("accept");
        break;
    }
```

---

## 8. 进程池（Pre‑fork）模式

### 8.1 为什么需要进程池

每次 `fork()` 一个进程处理客户端有一定开销：
- 页表拷贝（即使 COW，页表本身仍需复制）
- 内核数据结构分配
- 调度器元数据

对于高频连接场景（如每秒数千连接），每次 fork 的开销不可忽略。

### 8.2 Pre‑fork 架构

```c
#define N_WORKERS 4

int main() {
    int listen_fd = make_listen_socket(PORT);

    for (int i = 0; i < N_WORKERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* worker 子进程 */
            serve_loop(listen_fd);   // 多个 worker 同时 accept()
            exit(0);
        }
    }

    /* master 父进程：只负责监控和重启 */
    while (1) {
        pid_t dead = waitpid(-1, &status, 0);
        if (dead > 0) {
            // 重启退出的 worker
            pid_t new = fork();
            if (new == 0) {
                serve_loop(listen_fd);
                exit(0);
            }
        }
    }
}
```

### 8.3 进程池通信：信号 / 管道

Master 通过**信号**或**管道**通知 worker：

```c
// master → worker 的信号控制
#define SIG_RELOAD   SIGUSR1     // 重新加载配置
#define SIG_GRACEFUL SIGUSR2    // 优雅关闭

// 或者使用 socketpair / pipe
int worker_pipes[N_WORKERS][2];
for (int i = 0; i < N_WORKERS; i++) {
    socketpair(AF_UNIX, SOCK_DGRAM, 0, worker_pipes[i]);
}

// master 写入指令
write(worker_pipes[worker_id][0], "RELOAD", 6);

// worker 读取指令（与 accept 混用 — 需要 epoll/select）
```

### 8.4 实际案例：Apache 的 prefork MPM

Apache HTTPD 的 `prefork` 模式正是这种架构：

- **StartServers**：启动时的 worker 数
- **MinSpareServers / MaxSpareServers**：空闲 worker 数范围
- **MaxRequestWorkers**：最大并发 worker 数
- master 进程动态调整 worker 数量

```
Apache prefork 架构
    ┌────── master (PID 1) ──────┐
    │      监控 + 管理 worker      │
    └──────────┬─────────────────┘
               │ fork
    ┌──────────┼──────────┐
    ▼          ▼          ▼
  worker1    worker2    worker3
  accept()   accept()   accept()
```

---

## 9. 进程间通信（IPC）概要

多进程服务器中，不同 worker 进程之间或 worker 与 master 之间需要通信。以下是常用手段：

### 9.1 管道（Pipe） / 命名管道（FIFO）

```c
int pipe_fds[2];
pipe(pipe_fds);             // pipe_fds[0] 读端，pipe_fds[1] 写端

pid_t pid = fork();
if (pid == 0) {
    close(pipe_fds[1]);     // 子进程关闭写端
    read(pipe_fds[0], buf, sizeof(buf));
} else {
    close(pipe_fds[0]);     // 父进程关闭读端
    write(pipe_fds[1], data, len);
}
```

**局限**：半双工，只能用于有亲缘关系的进程。

### 9.2 Unix Domain Socket

```c
int srv = socket(AF_UNIX, SOCK_STREAM, 0);
struct sockaddr_un addr = {.sun_family = AF_UNIX};
strcpy(addr.sun_path, "/tmp/worker.sock");
bind(srv, (struct sockaddr*)&addr, sizeof(addr));
listen(srv, 5);
```

**优势**：全双工、API 与网络 socket 一致、支持传递文件描述符（`sendmsg()` + `SCM_RIGHTS`）。

### 9.3 共享内存（Shared Memory）

```c
// System V 风格
int shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
void *ptr  = shmat(shmid, NULL, 0);
// 所有有权限的进程可以读写这块内存
// ⚠️ 需要信号量 / 互斥锁同步
```

**优势**：零拷贝，性能最高。
**问题**：需要外部同步机制（信号量、互斥锁）。

### 9.4 信号（Signal）

适合轻量级控制消息（如 reload、shutdown），**不适合传递数据**。

### 9.5 多进程并发服务器中共享数据的实现方式

| 场景 | 推荐方案 |
|------|----------|
| 共享计数器（连接数统计） | 共享内存 + 原子操作 / 信号量 |
| worker 状态监控 | 共享内存 + 状态轮询 |
| master → worker 指令 | 信号 / 管道 |
| worker 间数据交换 | Unix Domain Socket |
| worker 间任务分发 | 共享任务队列（共享内存 + 信号量） |

---

## 10. 多进程 vs 多线程 vs 协程

### 10.1 对比

| 维度 | 多进程 | 多线程 (pthread) | 协程 (coroutine) |
|------|--------|-------------------|-------------------|
| **地址空间** | 完全隔离 | 共享 | 共享 |
| **上下文切换** | 内核态，开销大（~1–10µs） | 内核态，略小（~0.5–5µs） | 用户态，极小（~0.1µs） |
| **创建开销** | 大（需要页表、PCB） | 中（仅栈 + TCB） | 极小（仅栈） |
| **最大数量** | 几百～几千（看 RAM） | 几千～几万（看 RAM） | 几十万～百万 |
| **同步** | 需要 IPC（管道、shm） | 共享内存 + 锁 | 共享内存 + 锁 |
| **崩溃隔离** | ✅ 子进程崩溃不影响父进程 | ❌ 一个线程 segfault → 整个进程挂 | ❌ 一个协程挂 → 整个进程挂 |
| **CPU 利用** | 天然多核 | 天然多核 | 默认单线程（需多协程器） |
| **编程复杂度** | 较低（隔离好） | 中（锁、竞争条件） | 中（异步模型） |
| **调试难度** | 低（独立地址空间） | 中（竞态条件常见） | 中（回调地狱 / async 链） |

### 10.2 网络服务器的选择原则

```
高安全性 / 稳定性需求（每个客户端独立隔离）
    └─ 多进程（如 Chrome 浏览器、OpenSSH、Apache prefork）

高性能 / 高并发（大量短连接）
    └─ 多线程（如 Nginx worker、MySQL）

超高并发（C10M 场景，长连接）
    └─ 事件驱动 + 协程（如 Redis、Nginx、Go net/http、libuv）

混合架构
    └─ 多进程（隔离） + 每进程多线程 / 协程（并发）
       （如 Nginx: master + worker 进程，每个 worker 事件驱动）
```

### 10.3 混合架构示例

```c
// 多进程 + 每进程事件驱动（Nginx 风格）
for (int i = 0; i < N_WORKERS; i++) {
    pid_t pid = fork();
    if (pid == 0) {
        // worker 进程内部使用 epoll 事件循环
        event_loop(listen_fd);
        exit(0);
    }
}
```

---

## 11. 常见陷阱与最佳实践

### 11.1 文件描述符泄漏

```c
/* ❌ 错误 */
pid = fork();
if (pid == 0) {
    serve(conn_fd);
    // 忘记 close(listen_fd) — 子进程持有 listen_fd
    exit(0);
}
close(conn_fd);  // 父进程 close 了，但子进程的 conn_fd 引用仍有效
```

**最佳实践**：
- 子进程 `close(listen_fd)`，父进程 `close(conn_fd)`
- 在 `fork()` 前用 `FD_CLOEXEC` 标记不该被继承的文件描述符
- 使用 `fcntl(fd, F_SETFD, FD_CLOEXEC)`

### 11.2 信号处理陷阱

- 信号处理函数必须是 **async-signal-safe** → 不可使用 `printf()`、`malloc()`、`free()`
- 只能调用：`write()`、`waitpid()`、`exit()`、`signal()` 等少数
- **推荐做法**：信号 handler 只写一个标志位（`volatile sig_atomic_t`），在主循环中检查

```c
volatile sig_atomic_t g_child_exited = 0;

void handler(int sig) {
    g_child_exited = 1;   // ✅ 这是 async-signal-safe 的
}

int main() {
    while (1) {
        int conn_fd = accept(listen_fd, ...);
        if (g_child_exited) {
            while (waitpid(-1, NULL, WNOHANG) > 0);
            g_child_exited = 0;
        }
        // ...
    }
}
```

### 11.3 子进程未捕获 SIGPIPE

当进程向已关闭的 socket 写入时，内核发送 `SIGPIPE` 信号，其**默认行为是终止进程**。

```c
/* ❌ 子进程处理客户端时如果对方断开，write() 触发 SIGPIPE → 子进程无声消失 */
sigaction(SIGPIPE, &(struct sigaction){.sa_handler = SIG_IGN}, NULL);
// ✅ 在子进程启动时忽略 SIGPIPE，让 write() 返回 -1 + EPIPE
```

### 11.4 资源限制

- `ulimit -n` 控制每个进程最大 fd 数（默认 1024）
- `ulimit -u` 控制用户最大进程数
- `sysctl kernel.pid_max` 控制系统最大 PID 数
- /proc/sys/fs/file-max 控制系统级最大 fd 数

```bash
# 调整（临时）
ulimit -n 65535
sysctl -w kernel.pid_max=65536
```

### 11.5 优雅关闭（Graceful Shutdown）

服务器关闭时，不应暴力终止所有子进程，而应允许已建立的连接完成。

```c
volatile sig_atomic_t g_shutdown = 0;

void sigterm_handler(int sig) {
    g_shutdown = 1;
}

/* master 进程收到 SIGTERM */
if (g_shutdown) {
    // 通知所有 worker 停止接受新连接
    for (int i = 0; i < n_workers; i++) {
        write(worker_pipes[i][1], "SHUTDOWN", 8);
    }
    // 等待所有 worker 退出
    while (waitpid(-1, NULL, 0) > 0);
    close(listen_fd);
    exit(0);
}

/* worker 进程 */
void serve_loop(int listen_fd) {
    while (!g_shutdown) {
        // 设置 accept 超时，或使用 epoll + timerfd
        int conn_fd = accept(listen_fd, ...);
        if (conn_fd < 0 && errno == EINTR) continue;
        serve_client(conn_fd);
    }
}
```

### 11.6 地址重用：`SO_REUSEADDR`

```c
int opt = 1;
setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**为什么必须设置**：服务器重启时，`TIME_WAIT` 状态未结束的旧 socket 会阻止 `bind()`。`SO_REUSEADDR` 允许绑定到 `TIME_WAIT` 状态的端口。

---

## 12. 附录：关键系统调用速查

### 12.1 进程控制

| 系统调用 | 作用 | 关键点 |
|----------|------|--------|
| `fork()` | 创建子进程 | 一次调用两次返回；子进程返回 0 |
| `vfork()` | 轻量 fork | 父进程阻塞；子进程必须立即 exec 或 _exit |
| `clone()` | Linux 底层创建 | 可精细控制共享资源（线程本质是 `clone(CLONE_VM|...)`） |
| `exit()` | 终止进程 | 清理用户态资源，发出 SIGCHLD |
| `_exit()` | 不运行 atexit | 用于子进程 fork 后立即退出 |
| `wait()` | 回收任意子进程 | 阻塞 |
| `waitpid()` | 回收指定子进程 | 支持 WNOHANG |
| `execve()` | 替换进程映像 | fork + exec 是创建新程序的完整路径 |

### 12.2 信号处理

| 系统调用 | 作用 |
|----------|------|
| `signal()` | 设置信号处理（简化版，不建议新代码使用） |
| `sigaction()` | 设置信号处理（推荐，支持更多控制） |
| `sigprocmask()` | 阻塞/解除阻塞信号 |
| `sigpending()` | 查询未决信号 |
| `sigsuspend()` | 原子地设置信号掩码并等待信号 |
| `kill()` | 向进程发送信号 |

### 12.3 IPC

| 系统调用 | 类型 | 方向 |
|----------|------|------|
| `pipe()` | 管道 | 单向（有亲缘关系） |
| `mkfifo()` | 命名管道 | 单向（无亲缘关系也可） |
| `socketpair()` | 全双工 socket | 双向（有亲缘关系） |
| `shmget()` / `shmat()` | System V 共享内存 | 双向（无亲缘关系） |
| `mmap()` | POSIX 共享内存 | 双向 |
| `semget()` / `semop()` | System V 信号量 | 同步原语 |
| `sem_open()` / `sem_post()` | POSIX 信号量 | 同步原语 |
| `msgget()` / `msgsnd()` | System V 消息队列 | 双向 |

### 12.4 Socket API

| 系统调用 | 作用 |
|----------|------|
| `socket()` | 创建端点 |
| `bind()` | 绑定地址 |
| `listen()` | 监听（backlog 参数指定已完成连接队列长度） |
| `accept()` | 接受连接（阻塞 / 非阻塞） |
| `connect()` | 发起连接 |
| `send()` / `recv()` | 收发数据 |
| `sendmsg()` / `recvmsg()` | 高级 IO（可传递 fd、辅助数据） |
| `setsockopt()` | 设置选项（REUSEADDR、REUSEPORT、KEEPALIVE 等） |
| `getsockopt()` | 获取选项 |

---

## 总结

多进程并发服务器是 Unix/Linux 网络编程中最基础、最经典的并发模型，其核心理念可以用一句话概括：

> **`fork()` 让服务器可以同时服务多个客户端，而 COW 让 `fork()` 的代价变得可以接受。**

架构路径：

```
单进程迭代
    ↓  ❌ 无法并发
多进程（每次 fork）
    ↓  ✅ 隔离性好，⚠️ 创建开销
多进程 Pre‑fork（进程池）
    ↓  ✅ 预热加速，⚠️ 惊群
多进程 + SO_REUSEPORT
    ↓  ✅ 内核分发，无惊群
混合模型（多进程 + 每进程事件驱动/线程/协程）
    ↓  ✅ 隔离 + 高并发
多线程 / 事件驱动
    ↓  ⚠️ 灵活但复杂
```

**进一步学习方向**：
- [ ] 阅读 Linux 内核 `kernel/fork.c` 中 `copy_process()` 代码
- [ ] 实践：用 `perf` 分析 `fork()` 的开销分布
- [ ] 练习：实现一个带简单 HTTP 解析的多进程服务器
- [ ] 进阶：结合 `epoll` + `SO_REUSEPORT` 实现高性能 Web 服务器
- [ ] 参考：UNIX Network Programming (Stevens)、Understanding the Linux Kernel

---

> 📌 **如果本文档对你有帮助，欢迎 Star / 收藏。如有错误或补充，请提交 issue 或 PR。**

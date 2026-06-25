
# I/O 多路复用与高级 I/O — 从 select 到 epoll，组播与广播

> **目标读者**：已掌握基础 socket 和阻塞 I/O 编程，希望系统学习 I/O 多路复用（select / poll / epoll）、高级 I/O 操作以及 UDP 多播/广播的底层原理。
> **阅读路径**：从 I/O 模型分类开始 → select 内核实现 → poll 改进 → epoll 事件驱动 → 高级 I/O 函数 → 多播与广播的 socket 编程。

---

## 目录

1. [I/O 模型概述](#1-io-模型概述)
2. [`select()` — 可移植但低效](#2-select--可移植但低效)
3. [标准 I/O vs 系统 I/O（预备知识）](#3-标准-io-vs-系统-iop预备知识)
4. [`poll()` — select 的改进版](#4-poll--select-的改进版)
5. [`epoll()` — Linux 高并发利器](#5-epoll--linux-高并发利器)
6. [select / poll / epoll 深度对比](#6-select--poll--epoll-深度对比)
7. [非阻塞 I/O](#7-非阻塞-io)
8. [高级 I/O 函数](#8-高级-io-函数)
9. [信号驱动 I/O（SIGIO）](#9-信号驱动-iosigio)
10. [多播（Multicast）](#10-多播multicast)
11. [广播（Broadcast）](#11-广播broadcast)
12. [基于 epoll 的并发 echo 服务器](#12-基于-epoll-的并发-echo-服务器)
13. [常见陷阱与最佳实践](#13-常见陷阱与最佳实践)
14. [附录：关键系统调用速查](#14-附录关键系统调用速查)

---

## 1. I/O 模型概述

### 1.1 Unix 五种 I/O 模型

Stevens 在《UNIX Network Programming》中将 I/O 模型分为五类（从低到高）：

| 模型 | 阻塞阶段 | 特点 | 典型场景 |
|------|---------|------|---------|
| **阻塞 I/O** | 数据就绪 + 数据拷贝 | 最直观，进程睡眠等待 | 简单单客户端 |
| **非阻塞 I/O** | 只阻塞数据拷贝 | 轮询检查，浪费 CPU | 极少单独使用 |
| **I/O 多路复用** | select/poll/epoll + 数据拷贝 | 一次等待多个 fd | **高并发网络服务器** |
| **信号驱动 I/O (SIGIO)** | 只阻塞数据拷贝 | 通知式，回调读取 | 极少用，异步信号安全限制 |
| **异步 I/O (AIO)** | 完全不阻塞 | 内核完成全部操作后通知 | Linux AIO / io_uring |

### 1.2 为什么需要 I/O 多路复用

回顾上一篇文章中的单进程迭代服务器问题：

```c
while (1) {
    int conn = accept(listen_fd, ...);   // 阻塞等待连接
    serve_client(conn);                   // 阻塞处理请求
    close(conn);
    // 在 serve_client 期间，无法接受新连接！
}
```

多进程方案用 `fork()` 解决了这个问题，但带来了新问题：
- 进程开销大（即使 COW 也有页表拷贝）
- 进程数受 PID 限制和内存限制

**I/O 多路复用的核心思想**：**用一个进程/线程同时等待多个 I/O 事件**，内核告诉我们哪个 fd 就绪，再去处理它。

```
单线程 + I/O 多路复用:
           ┌─────────────────────────┐
客户端1 ───│                         │
客户端2 ───│── select/poll/epoll ────│── 事件循环
客户端3 ───│                         │
客户端4 ───│                         │
           └─────────────────────────┘
              ↑ 一个线程等待所有客户端
              ↑ 哪个就绪处理哪个
```

---

## 2. 标准 I/O vs 系统 I/O（预备知识）

在进入 select/poll/epoll 之前，先弄清一个基础但关键的问题：**C 标准库的 `FILE *` 和系统调用 `read/write` 到底有什么区别？**

**这是一个经典的历史问题，根源在于 Unix 的"一切皆文件"哲学与用户态缓冲的取舍。**

### 2.1 两个层次

```
应用程序
    │
    ├─ 标准 I/O (stdio)              ← C 标准库 (glibc)
    │    fopen / fread / fwrite / fclose
    │    FILE *  (用 FILE 指针操作)
    │    有用户态缓冲
    │
    ├─ 低级 I/O (系统调用)            ← 内核接口
    │    open / read / write / close
    │    int fd  (文件描述符)
    │    无缓冲（直接进入内核）
    │
    └─ 内核
         VFS (虚拟文件系统) → 具体文件系统 / 设备驱动
```

| 层次 | 接口 | 数据类型 | 缓冲 | 所在位置 |
|------|------|---------|------|---------|
| **标准 I/O** | `fopen`, `fread`, `fwrite`, `fprintf`, `fgets` | `FILE *` | ✅ **用户态缓冲** | C 库 (glibc) |
| **系统 I/O** | `open`, `read`, `write`, `close`, `lseek` | `int fd` | ❌ 无用户态缓冲 | 内核 (Linux) |

### 2.2 文件描述符（fd）是什么

文件描述符是内核返回给用户态的一个**非负整数**，本质是**进程文件描述符表的索引**。

```
进程的 task_struct
    │
    └─ files_struct (打开的文件表)
         │
         ├─ fd 0 → struct file *  → /dev/pts/0 (stdin)
         ├─ fd 1 → struct file *  → /dev/pts/0 (stdout)
         ├─ fd 2 → struct file *  → /dev/pts/0 (stderr)
         ├─ fd 3 → struct file *  → socket(监听端口)
         ├─ fd 4 → struct file *  → socket(已连接)
         └─ ...
```

**关键理解**：
- `fd` 只是一个整数索引 (0, 1, 2, 3, ...)
- 内核通过 `fd` 在进程的文件表中找到对应的 `struct file`
- `struct file` 指向一个 `struct inode`（文件）或 `struct sock`（socket）
- 每次 `fork()` 后，子进程拷贝父进程的文件表——**父子进程的 fd 虽然数字相同，但各自指向不同的文件表项**？不对！`fork()` 是**浅拷贝**——父子进程共享相同的 `struct file`（引用计数 +1）

### 2.3 `FILE *` 是什么

```c
// glibc 内部定义（简化）
struct _IO_FILE {
    int _fileno;          // ★ 底层的文件描述符
    unsigned char *_IO_read_ptr;   // 读指针当前位置
    unsigned char *_IO_read_end;   // 读缓冲区结束位置
    unsigned char *_IO_read_base;  // 读缓冲区起始
    unsigned char *_IO_write_ptr;  // 写指针当前位置
    unsigned char *_IO_write_end;  // 写缓冲区结束位置
    unsigned char *_IO_write_base; // 写缓冲区起始
    unsigned char *_IO_buf_base;   // 缓冲区基址
    unsigned char *_IO_buf_end;    // 缓冲区结束
    int _flags;           // 标志位（缓冲模式、读写模式等）
    // ...
};
typedef struct _IO_FILE FILE;
```

**`FILE *` 本质是一个结构体指针，里面封装了**：
1. 底层文件描述符 `_fileno`（一定有）
2. 用户态缓冲区（读写各一个，或共享）
3. 缓冲区状态（有多少数据、当前读写位置等）
4. 操作标志（读/写/追加、错误/结束等）

```
FILE *fp = fopen("a.txt", "r");
                     │
                     ▼
        ┌──────────────────────────────┐
        │  _IO_FILE (FILE 结构体)      │
        │  _fileno = 3                 │ ← 最终还是会 open 得到一个 fd
        │  _IO_read_ptr → ┌──┬──┬──┐  │
        │  _IO_read_end → │ H│e│l│l│o│ │ ← 用户态缓冲区 (glibc malloc)
        │  _IO_buf_base → └──┴──┴──┘  │   一次性 read(fd, buf, 4096)
        │  _flags = ...                │   然后 fgetc 从缓冲区拿
        └──────────────────────────────┘
```

### 2.4 三种缓冲模式

`setvbuf()` / `setbuf()` / `setlinebuf()` 控制：

| 模式 | 宏 | 行为 | 默认适用 |
|------|-----|------|---------|
| **全缓冲** (fully buffered) | `_IOFBF` | 缓冲区满了（通常 4096 或 8192 字节）才真正 write | **普通文件** |
| **行缓冲** (line buffered) | `_IOLBF` | 遇到 `\n` 或缓冲区满才真正 write | **终端 (stdout)** |
| **无缓冲** (unbuffered) | `_IONBF` | 每次调用都立即 write | **stderr**（确保错误立即输出） |

```c
setvbuf(stdout, NULL, _IOLBF, 0);    // stdout 设为行缓冲
setvbuf(stderr, NULL, _IONBF, 0);    // stderr 设为无缓冲

char buf[8192];
setvbuf(fp, buf, _IOFBF, sizeof(buf)); // 自定义全缓冲
```

### 2.5 标准 I/O vs 系统 I/O 详细对比

```c
// ─── 系统 I/O ───
int fd = open("file.txt", O_RDONLY);       // 系统调用（陷入内核）
char buf[1024];
ssize_t n = read(fd, buf, sizeof(buf));    // 系统调用（陷入内核）
// ★ 每次 read/write 都是一次系统调用 → 用户态↔内核态切换

// ─── 标准 I/O ───
FILE *fp = fopen("file.txt", "r");         // 内部调用 open() 一次 + malloc 缓冲区
char c = fgetc(fp);                         // 大多数情况下不调用 read()
// ★ fgetc/fputc 通常只在用户态操作缓冲区
// ★ 仅当缓冲区空了/满了才调用一次 read()/write()
```

| 维度 | 系统 I/O (read/write) | 标准 I/O (fread/fwrite) |
|------|----------------------|------------------------|
| **参数类型** | `int fd`（文件描述符） | `FILE *`（封装了 fd + 缓冲区） |
| **缓冲** | **无用户态缓冲** | **有用户态缓冲**（可控制模式） |
| **系统调用次数** | 每次读写都调用 | 缓冲满/空时才调用 |
| **是否可移植** | POSIX 标准（Unix/Linux） | **C 语言标准（全平台）** |
| **二进制安全** | ✅ 安全 | ✅ 安全（`"rb"`/`"wb"` 模式） |
| **格式化 I/O** | ❌ 需要自己 `sprintf`+`write` | ✅ `fprintf`/`fscanf` 内置 |
| **行输入** | ❌ 需要自己处理 `\n` | ✅ `fgets` 方便 |
| **随机访问** | `lseek(fd, offset, whence)` | `fseek(fp, offset, whence)` |
| **socket 使用** | ✅ **可直接用** | ❌ **不能直接用于 socket** |
| **性能（大量小 IO）** | ❌ 慢（频繁系统调用） | ✅ 快（缓冲减少系统调用） |
| **性能（大块 IO）** | ✅ 接近（可自己设大 buf） | ✅ 相当 |
| **文件描述符** | 直接拥有 | 可通过 `fileno(fp)` 获取 |
| **跨 fork 行为** | fd 可继承 | `FILE *` 的缓冲区可能双倍 flush |

### 2.6 socket 场景——标准 I/O 的致命缺陷

**这是网络编程中最容易踩的坑之一**——**标准 I/O 不能用于 socket，至少不能"直接"用于**。

```c
// ❌ 错误：用 fread/fwrite 操作 socket

int conn_fd = accept(listen_fd, NULL, NULL);

// 错误用法 1:
FILE *fp = fdopen(conn_fd, "r+");   // fdopen 把 fd 包装成 FILE *
fprintf(fp, "Hello\n");
fclose(fp);              // ★ fclose 会 close(conn_fd) — 对方连接被关闭！
close(conn_fd);          // ★ 双重 close → 灾难

// 错误用法 2:
fread(buf, 1, 100, fp);  // ★ fread 可能一次只读一点点（依赖缓冲区状态）
// 对于 socket，"读多少"由发送方决定，不是由缓冲区大小决定
// fread 无法处理部分到达的数据包边界

// 错误用法 3:
fprintf(fp, "data");
// ★ 行缓冲时，如果不带 \n，数据留在缓冲区不发送
// ★ 全缓冲时，要等 4K 满了才发送
// → 对方永远等不到数据！
```

**推荐做法——网络编程只用系统 I/O**：

```c
// ✅ 正确：直接用 read/write
write(conn_fd, "data", 4);
// 每次立即发送（假设 socket 默认阻塞模式）

// 或者用 send/recv（更精细控制）
send(conn_fd, "data", 4, MSG_NOSIGNAL);
```

**标准 I/O 不适应 socket 的根本原因**：

| 标准 I/O 的假设 | socket 的现实 | 冲突 |
|----------------|---------------|------|
| 文件有固定长度，可以 lseek 随机访问 | socket 是流，不能 lseek | ❌ |
| 缓冲区满了再 flush 是安全的 | 实时通信要求数据立即发出 | ❌ |
| `fclose` 应关闭底层 fd | socket 可能被多个 fd 引用 | ❌ |
| `fread` 读到期望的字节数为止 | socket 可能只收到部分数据 | ❌（block） |
| `feof`/`ferror` 可以判断结束 | socket 关闭是 `read=0` 或 `EPOLLRDHUP` | ❌ |

### 2.7 `fdopen` / `fileno` — 两个世界的桥梁

尽管不推荐在 socket 上直接用 stdio，但了解这两个函数有助于理解两者的关系：

```c
FILE *fdopen(int fd, const char *mode);
// 作用：把一个 fd "包装"成 FILE *
// 常用于：把 socket fd 包装后给 fprintf 输出日志
// ⚠️ fclose 会 close 底层 fd！不想关闭的话用 fclose 前 dup 一份

int fileno(FILE *stream);
// 作用：从 FILE * 取出底层的 fd
// 常用于：需要同时用 stdio（方便格式化）和系统调用（细粒度控制）

// 典型场景：用 fprintf 方便地格式化输出到文件/socket
FILE *fp = fdopen(conn_fd, "w");
setbuf(fp, NULL);           // ★ 设为无缓冲，否则数据会滞留
fprintf(fp, "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\n\r\n", len);
fflush(fp);                 // ★ 手动 flush
// ⚠️ 不要 fclose(fp)，用 shutdown(conn_fd, SHUT_WR) 关闭写半部
shutdown(conn_fd, SHUT_WR);
```

### 2.8 `fflush()` — 强制刷新缓冲区

#### 作用

`fflush(FILE *fp)` 强制将 `fp` 的用户态缓冲区内容**立即写入内核**（即调用 `write()` 系统调用）。

```c
int fflush(FILE *fp);
// 返回：0 成功，EOF 失败
// fp = NULL → 刷新所有打开的输出流
```

#### 何时需要手动 fflush

```c
// ─── 场景 1：行缓冲但不想等 \n ───
fprintf(stdout, "Processing...");   // 无 \n → 行缓冲模式下不输出
fflush(stdout);                     // ★ 强制立即输出
// 常用于：进度条、交互式提示

// ─── 场景 2：程序可能崩溃前确保数据写出 ───
fprintf(log_fp, "Critical operation start\n");
fflush(log_fp);                     // ★ 即使后面崩溃，日志已落盘
// dangerous_operation();           // 如果这里 segfault，数据已写出

// ─── 场景 3：fdopen 包装的 socket ───
FILE *fp = fdopen(conn_fd, "w");
setbuf(fp, NULL);                   // 或者用 fflush 手动控制
fprintf(fp, "HTTP/1.1 200 OK\r\n");
fflush(fp);                         // ★ 确保 HTTP 头立即发出
```

#### fflush 不适用于输入流

```c
// C 标准规定 fflush 用于输入流是"未定义行为"
// 但 Linux glibc 扩展允许 fflush(stdin) 丢弃已读入缓冲的未读数据
fflush(stdin);      // Linux 上：丢弃 stdin 缓冲区中还未被读取的数据
                    // 不是所有平台都支持！（不可移植）
```

#### fflush 的内核路径

```
fprintf(fp, "hello");   // 数据写入用户态缓冲区
    │                   // 缓冲区未满，不调用 write()
    ▼
fflush(fp);             // ★ 强制 flush
    │
    └─ _IO_flush_all()
         └─ _IO_OVERFLOW(fp, EOF)
              └─ write(fp->_fileno, fp->_IO_write_base,
                        fp->_IO_write_ptr - fp->_IO_write_base)
                   │
                   └─ ★ 系统调用：用户态→内核态
                        数据进入内核缓冲区 → 最终到磁盘/网络
```

#### fflush(NULL) 的特殊行为

```c
fflush(NULL);   // 刷新所有打开的输出流
// 等价于：
//   fflush(stdout);
//   fflush(stderr);
//   fflush(log_fp);
//   fflush(all_other_output_files);
```

### 2.9 `feof()` / `ferror()` / `clearerr()` — 流状态查询

#### `feof()` — 判断是否到达文件结尾

```c
int feof(FILE *fp);
// 返回：非 0 表示已到达文件结尾，0 表示未到达
```

**⚠️ 最常见的误解：`feof` 不是"预判"函数，而是"事后"函数。**

```c
// ❌ 错误用法
FILE *fp = fopen("data.bin", "r");
while (!feof(fp)) {             // ★ 问题：此时还没读，feof 返回 0
    fread(buf, 1, 100, fp);     // 读到最后一次时，读了 0 字节
    // 处理 buf — ★ 但这次没读到新数据！上一次的数据被重复处理
}

// ✅ 正确用法
while (1) {
    size_t n = fread(buf, 1, 100, fp);
    if (n == 0) {               // ★ 先检查本次读了多少
        if (feof(fp)) {         // 到达文件尾，正常结束
            break;
        }
        if (ferror(fp)) {       // 发生错误
            perror("fread");
            break;
        }
    }
    // 处理 buf 中的 n 字节
}
```

**`feof` 返回非 0 的条件**：上一次读操作因为**到达文件尾而返回 0**。换句话说：`feof` 是在读操作失败之后才能判断的，不是提前预知的。

```
文件内容: [H][e][l][l][o]

初始: feof(fp) = 0

fgetc(fp) → 'H'    feof = 0
fgetc(fp) → 'e'    feof = 0
fgetc(fp) → 'l'    feof = 0
fgetc(fp) → 'l'    feof = 0
fgetc(fp) → 'o'    feof = 0
fgetc(fp) → EOF    feof = 1   ← 只有读操作返回了 EOF 之后，feof 才变成 1
```

#### `ferror()` — 判断是否出错

```c
int ferror(FILE *fp);
// 返回：非 0 表示流发生了错误
```

```c
FILE *fp = fopen("data.bin", "r");
fread(buf, 1, 100, fp);
if (ferror(fp)) {
    printf("Read error: %s\n", strerror(errno));
    clearerr(fp);       // 清除错误标志（见下文）
}
```

**常见触发场景**：
- 从 socket 读取时对方重置连接（`ECONNRESET`）
- 读取设备文件时硬件错误
- 向磁盘写满时写入

#### `clearerr()` — 清除 feof 和 ferror 标志

```c
void clearerr(FILE *fp);
// 清除 fp 的 EOF 标志和错误标志
```

**为什么需要它？**

```c
// ─── 场景：日志文件滚动 ───
FILE *log_fp = fopen("server.log", "a");
while (1) {
    if (fprintf(log_fp, "...") < 0) {
        // 写入失败 → ferror 被设置
        // 原因可能是磁盘满了
        // 等待后重试...
        sleep(10);
        clearerr(log_fp);           // ★ 必须清除错误标志
        // 否则后续 fprintf 全部静默失败（错误标志阻止一切写操作）
    }
}
```

**原理**：一旦 `ferror` 被设置，所有后续的读写操作都会**立即返回失败**，直到调用 `clearerr()` 清除它。

#### feof / ferror / clearerr 状态位图

```
FILE *fp 的内部状态（只取相关位）:

  _flags:
    ┌───┬───┬───┬───┬───┬───┬───┬───┐
    │   │   │   │ E │ F │   │   │   │
    │   │   │   │ R │ E │   │   │   │
    │   │   │   │ R │ O │   │   │   │
    │   │   │   │   │ F │   │   │   │
    └───┴───┴───┴───┴───┴───┴───┴───┘
                    ↑   ↑
                    │   └─ _IO_EOF_SEEN (0x10)  ← feof 检查此位
                    └───── _IO_ERR_SEEN (0x20)  ← ferror 检查此位

clearerr(fp): 将这两个位清零

fclose(fp): 自动调用 clearerr
```

#### 网络编程中 feof 为什么不能用

```c
// ❌ ❌ ❌ 网络编程绝对不能用 feof 判断 socket 关闭
FILE *fp = fdopen(conn_fd, "r");
char buf[1024];
fread(buf, 1, sizeof(buf), fp);   // read 返回 0 → feof 置位
if (feof(fp)) {
    // 对方关闭了连接
    // ★ 但问题来了：如果对方这次只发了部分数据，fread 返回 > 0
    //   feof 仍然是 0！你无法区分"数据还没到"和"对方已关闭"
}

// ✅ socket 中间的正确做法
ssize_t n = read(conn_fd, buf, sizeof(buf));
if (n == 0) {
    // 对方关闭（read 返回 0 是唯一可靠的判断方式）
}
if (n < 0) {
    if (errno == EAGAIN) {
        // 非阻塞模式下无数据，不是错误
    }
    // 其他错误
}
```

#### 三个函数的总结

| 函数 | 作用 | 何时为真 | 如何清除 |
|------|------|---------|---------|
| `feof(fp)` | 检查是否到达文件尾 | 上一次读操作返回 0（因 EOF） | `clearerr(fp)` 或 `fseek(fp)` |
| `ferror(fp)` | 检查是否发生 I/O 错误 | 上一次读写操作返回错误 | `clearerr(fp)` |
| `clearerr(fp)` | 清除 feof 和 ferror 标志 | — | — |

---

### 2.10 性能差异的实际测试

```c
// 测试：写 100 万行 "Hello, World!\n" 到文件

// 系统 I/O：
int fd = open("test.txt", O_WRONLY | O_CREAT, 0644);
for (int i = 0; i < 1000000; i++) {
    write(fd, "Hello, World!\n", 14);   // ★ 100 万次系统调用
}
// 实测：约 3-5 秒（系统调用开销巨大）

// 标准 I/O：
FILE *fp = fopen("test.txt", "w");
for (int i = 0; i < 1000000; i++) {
    fprintf(fp, "Hello, World!\n");     // ★ 只在用户态缓冲区操作
}
// 实测：约 0.3-0.5 秒（缓冲区 4096 字节满才写一次 → 约 3500 次系统调用）
```

**结论**：对于**小数据频繁 IO**，标准 I/O 比系统 I/O 快 **10 倍以上**。

### 2.9 网络编程中的实践建议

```c
// ─── 服务端：只使用系统 I/O（read/write 或 send/recv）───
int conn = accept(fd, ...);
ssize_t n = read(conn, buf, sizeof(buf));
write(conn, buf, n);
// socket 场景用系统 I/O，简单直接

// ─── 日志输出：建议用标准 I/O ───
FILE *log_fp = fopen("server.log", "a");
fprintf(log_fp, "Connection from %s:%d\n", ip, port);
// 日志文件用标准 I/O，利用缓冲减少磁盘写入

// ─── 错误输出：stderr 默认无缓冲 ───
fprintf(stderr, "ERROR: %s\n", strerror(errno));
// 错误消息立即输出，不依赖缓冲

// ─── 配置文件读取：标准 I/O 方便 ───
FILE *conf = fopen("server.conf", "r");
char line[256];
while (fgets(line, sizeof(line), conf)) {
    parse_config(line);
}
```

### 2.10 历史视角——为什么会有两套 I/O

```
1970s: Unix 诞生 → open/read/write/close（系统调用）
       用户态需要缓冲来减少昂贵的内核切换
       → 每个程序员自己实现缓冲（代码重复）

1980s: ANSI C 标准委员会将缓冲 I/O 标准化
       → fopen/fread/fwrite/fclose → FILE *
       → 程序员不用再自己实现缓冲了

1990s: 网络编程流行
       → 发现 FILE * 不适合 socket（缓冲导致数据滞留）
       → 但文件场景仍大量使用标准 I/O

Today:  文件 → 标准 I/O（性能好，方便）
        socket → 系统 I/O（read/write / 多路复用）
        混合 → fdopen + setbuf(fp, NULL)（小心使用）
```

### 2.11 总结对照表

```
问题："我应该用 FILE * 还是 fd？"

如果是操作普通磁盘文件:
    ├─ 读写大量小数据 → 标准 I/O（fread/fwrite/fprintf）
    ├─ 读大块数据 → 两者都可（stdin 默认行缓冲需注意）
    ├─ 二进制文件 → 标准 I/O 以 "rb"/"wb" 打开
    └─ 需要 mmap 映射 → 系统 I/O（mmap 需要 fd）

如果是操作 socket/pipe/设备:
    ├─ 一律用系统 I/O（read/write/send/recv）
    ├─ 例外：配合 fdopen + setbuf(NULL) 用于格式化日志输出
    └─ 绝对不要 fclose() 包装后的 socket FILE *
```

---

## 3. `select()` — 可移植但低效

### 2.1 函数签名

```c
#include <sys/select.h>

int select(
    int nfds,                     // 最大 fd 编号 + 1
    fd_set *readfds,              // 监视可读的 fd 集合
    fd_set *writefds,             // 监视可写的 fd 集合
    fd_set *exceptfds,            // 监视异常的 fd 集合
    struct timeval *timeout       // 超时时间（NULL = 无限等待）
);
// 返回：就绪的 fd 数量，0 = 超时，-1 = 出错
```

### 2.2 `fd_set` 的位图实现与限制

`fd_set` 在内核/用户态中是一个**固定长度的位图**：

```c
// 内核定义 /usr/include/sys/select.h
#define FD_SETSIZE  1024                    // 默认可监视的最大 fd 数
typedef struct {
    unsigned long fds_bits[FD_SETSIZE / 8 / sizeof(long)];
    // = 1024 / 8 / 8 = 16 个 unsigned long（64位系统）
    // 共 1024 位，每一位代表一个 fd
} fd_set;
```

```
位图结构（以 16 位示意）:
  位:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
      ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
      │ 1│ 1│ 0│ 0│ 0│ 1│ 0│ 0│ 0│ 0│ 0│ 0│ 0│ 0│ 0│ 0│
      └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
      ↑ fd=0       ↑ fd=5
      (stdin)      (socket)
```

**操作宏**（全部是位运算）：

```c
#define FD_SET(fd, set)     ((set)->fds_bits[fd / NFDBITS] |=  (1UL << (fd % NFDBITS)))
#define FD_CLR(fd, set)     ((set)->fds_bits[fd / NFDBITS] &= ~(1UL << (fd % NFDBITS)))
#define FD_ISSET(fd, set)   ((set)->fds_bits[fd / NFDBITS] &   (1UL << (fd % NFDBITS)))
#define FD_ZERO(set)        memset(set, 0, sizeof(fd_set))
```

### 2.3 select 的核心流程

```
用户态                                 内核态
────────────────                     ────────────────
FD_ZERO(&readfds)
FD_SET(listen_fd, &readfds)

select(FD_SETSIZE, &readfds, ...)
  │
  ├─ sys_select() → core_sys_select()
  │    ├─ 拷贝 readfds 位图到内核 (copy_from_user)
  │    ├─ 遍历所有被设置的 fd (0 ~ nfds-1):
  │    │    for (fd = 0; fd < nfds; fd++) {
  │    │        if (!FD_ISSET(fd, &readfds)) continue;
  │    │        sock = sockfd_lookup(fd);      ← 通过 fd 找 socket
  │    │        if (sock->ops->poll(sock))     ← 调用 poll 方法检查状态
  │    │            FD_SET(fd, &res_readfds);  ← 就绪则标记
  │    │    }
  │    │    如果没有 fd 就绪:
  │    │        ├─ poll_initwait(&table)       ← 注册回调到等待队列
  │    │        ├─ __pollwait()                ← 将当前进程加入各 socket 的等待队列
  │    │        └─ schedule_timeout()          ← 让出 CPU，休眠
  │    │            └─ 超时或某 fd 就绪 → 唤醒
  │    │                 └─ 再次遍历检查哪 fd 就绪
  │    │
  │    └─ 将 res_readfds 拷贝回用户态 (copy_to_user)
  │
  └─ 返回就绪 fd 数量
       │
       ├─ FD_ISSET(listen_fd, &readfds) → accept() ← 新连接
       └─ FD_ISSET(conn_fd, &readfds) → read()   ← 已有数据
```

### 2.4 select 的三个主要缺陷

| 缺陷 | 原因 | 影响 |
|------|------|------|
| **fd 数量限制** | `fd_set` 大小硬编码为 `FD_SETSIZE`（1024） | 无法处理超过 1024 的并发连接（除非重编内核） |
| **O(n) 遍历** | 每次都要遍历 nfds 范围内的**所有** fd，不管有没有设置 | nfds=1024 时，每次调用扫描 1024 位 → CPU 缓存污染 |
| **内核态/用户态两次拷贝** | 每次调用都要把整个 `fd_set` 从用户态拷到内核，再拷回来（约 128 字节） | 虽然字节数不大，**但用户态也要在调用前后重新设置 FD_SET** |
| **fd_set 被修改** | select 返回后，readfds 被改写（只剩就绪的 fd） | 每次调用前必须重新 FD_SET 所有感兴趣的 fd（最烦人） |

### 2.5 select 示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {.sin_family = AF_INET,
                               .sin_addr.s_addr = INADDR_ANY,
                               .sin_port = htons(PORT)};
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 5);

    fd_set readfds, allfds;     // allfds: 保存所有感兴趣的 fd
    int max_fd = listen_fd;
    FD_ZERO(&allfds);
    FD_SET(listen_fd, &allfds);

    printf("select-based server on port %d\n", PORT);

    while (1) {
        readfds = allfds;               // ★ 每次必须重新赋值（select 会修改）
        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0) { perror("select"); break; }

        /* 检查 listen_fd（新连接） */
        if (FD_ISSET(listen_fd, &readfds)) {
            int conn = accept(listen_fd, NULL, NULL);
            FD_SET(conn, &allfds);          // 加入监听集合
            if (conn > max_fd) max_fd = conn;
            printf("New connection, fd=%d\n", conn);
        }

        /* 检查所有客户端 fd */
        for (int fd = 0; fd <= max_fd; fd++) {
            if (fd == listen_fd) continue;
            if (FD_ISSET(fd, &readfds)) {
                char buf[1024] = {0};
                ssize_t n = read(fd, buf, sizeof(buf));
                if (n <= 0) {               // 断开或错误
                    close(fd);
                    FD_CLR(fd, &allfds);
                    printf("Client fd=%d disconnected\n", fd);
                } else {
                    write(fd, buf, n);      // echo
                }
            }
        }
    }
    return 0;
}
```

---

## 4. `poll()` — select 的改进版

### 3.1 函数签名

```c
#include <poll.h>

struct pollfd {
    int   fd;          // 要监视的文件描述符（设为 -1 则忽略该项）
    short events;      // 感兴趣的事件（输入参数）
    short revents;     // 实际发生的事件（输出参数，由内核填写）
};

int poll(
    struct pollfd *fds,    // pollfd 数组
    nfds_t nfds,           // 数组长度
    int timeout            // 超时（毫秒，-1 = 无限等待，0 = 立即返回）
);
// 返回：就绪的 fd 数量，0 = 超时，-1 = 出错
```

### 3.2 poll vs select 的改进

| 方面 | select | poll |
|------|--------|------|
| **数据结构** | 位图（`fd_set`），最多 1024 | 数组（`struct pollfd[]`），**无硬上限** |
| **fd 管理** | 每次调用前重建位图，返回后位图被改写 | **events 和 revents 分离** → 无需重建输入 |
| **监听类型** | 只有 read/write/error 三类 | 精确到 **16 种事件**（POLLIN, POLLOUT, POLLERR, POLLHUP, POLLRDHUP …） |
| **nfds 含义** | 最大 fd 编号 + 1（受限于监听范围内的最高 fd） | 数组元素数量（无关 fd 具体值） |
| **内存拷贝** | 拷贝整个位图（128 字节） | 拷贝整个数组（`nfds * sizeof(pollfd)`） |
| **遍历方式** | 遍历 0 ~ nfds-1 所有 fd | 遍历 fds[0] ~ fds[nfds-1] |

### 3.3 `pollfd` 的事件标志

```c
// 输入事件 (events)
POLLIN      // 数据可读（包括 TCP FIN / 数据到达）
POLLOUT     // 可写入（发送缓冲区有空间）
POLLPRI     // 带外数据（urgent data）

// 输出事件 (revents)
POLLIN      // 与输入相同
POLLOUT     // 与输入相同
POLLERR     // 发生错误（自动设置，无需在 events 中注册）
POLLHUP     // 连接挂断（对方 close 或 shutdown write）
POLLRDHUP  // 连接对端关闭（Linux 2.6.17+，比 POLLHUP 更精细）
POLLNVAL    // fd 未打开（自动设置）
```

**关闭连接的检测**：

```c
// select 风格
if (FD_ISSET(fd, &readfds)) {
    n = read(fd, buf, sizeof(buf));
    if (n == 0) { /* 对方关闭 */ }      // read 返回 0 才知道
}

// poll 风格 — 可以提前判断
if (fds[i].revents & POLLRDHUP) {       // ★ 对方关闭连接（不用 read 就知道）
    close(fds[i].fd);
    fds[i].fd = -1;                      // 标记为无效
}
if (fds[i].revents & POLLIN) {
    n = read(fds[i].fd, buf, sizeof(buf));
}
```

### 3.4 poll 的内核实现

```
poll(fds, nfds, timeout)
  │
  ├─ sys_poll()
  │    ├─ copy_from_user(fds, nfds * sizeof(struct pollfd))  ← 拷贝到内核
  │    ├─ do_poll():
  │    │    for (每个 pollfd 项):
  │    │        if (fd == -1) continue;
  │    │        sock = fget(fd)                ← 通过 fd 获取 file 结构
  │    │        mask = sock->f_op->poll(sock)  ← 调用 poll 方法
  │    │        if (mask) {
  │    │            revents = events & mask;   ← 只返回感兴趣的
  │    │            count++;
  │    │        }
  │    │    如果 count == 0 && timeout != 0:
  │    │        ├─ poll_schedule_timeout()     ← 休眠
  │    │        └─ 唤醒后再次遍历
  │    │
  │    └─ copy_to_user(fds)  ← 把 revents 拷回用户态（与输入共用同一内存）
  │
  └─ 返回 count
```

### 3.5 poll 的根本问题

虽然 poll 去掉了 1024 限制和 fd_set 的重建麻烦，但**核心问题未变**：

- **仍然是 O(n) 遍历**：每次调用都要遍历所有 N 个 fd
- **仍然是全量拷贝**：N 个 `struct pollfd`（每个 8 字节）在用户态和内核态之间来回拷
- **随着 fd 数量增长，性能线性下降**

当 N = 1000 时，select 和 poll 性能差不多。
当 N = 10000 时，poll 慢到不可用——这就是 **C10K 问题**的核心所在。

---

## 5. `epoll()` — Linux 高并发利器

### 4.1 epoll 的设计哲学

epoll 针对 select/poll 的三个根本问题，从设计上做了颠覆：

| select/poll 的问题 | epoll 的解决 |
|-------------------|-------------|
| **O(n) 遍历**：每次把全部 fd 传给内核，内核遍历 | **O(1) 回调**：内核只告诉你**哪些 fd 就绪**，不遍历 |
| **全量拷贝**：每次调用都在用户态/内核态传整个集合 | **注册一次，多次复用**：`epoll_ctl` 注册 fd，`epoll_wait` 只取结果 |
| **无记忆**：内核不记住用户关心哪些 fd | **红黑树 + 就绪链表**：内核记住所有注册的 fd |

### 4.2 三个函数

```c
#include <sys/epoll.h>

/* 1. 创建 epoll 实例 */
int epoll_create(int size);             // 旧版，size 已废弃
int epoll_create1(int flags);           // 新版，flags=0 或 EPOLL_CLOEXEC
// 返回：epfd（epoll 实例的文件描述符）

/* 2. 注册/修改/删除 fd 事件 */
int epoll_ctl(
    int epfd,                // epoll 实例 fd
    int op,                  // EPOLL_CTL_ADD / EPOLL_CTL_MOD / EPOLL_CTL_DEL
    int fd,                  // 要监视的 fd
    struct epoll_event *event  // 关心的事件 + 用户数据
);

/* 3. 等待事件 */
int epoll_wait(
    int epfd,
    struct epoll_event *events,  // 输出：就绪的事件数组
    int maxevents,               // events 数组容量
    int timeout                  // 超时毫秒，-1 无限等待
);
// 返回：就绪的 fd 数量，0 = 超时，-1 = 出错
```

### 4.3 数据结构关系

```
用户态                                    内核态
─────                                    ─────
epoll_create(0)
  └─ 返回 epfd ─────────────────────────→ struct eventpoll
                                           ├─ wq (等待队列)     ← epoll_wait 休眠在此
                                           ├─ rdllist (就绪链表) ← 就绪的 fd 列表
                                           └─ rbr (红黑树根节点) ← 所有注册的 fd

epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev)
                                          ├─ ep_insert()
                                          │    ├─ 创建 struct epitem
                                          │    ├─ 插入红黑树 (rbr)   ← 以 fd 为 key
                                          │    └─ 注册回调到 fd 的等待队列:
                                          │         └─ ep_poll_callback()
                                          │              └─ 当 fd 就绪时:
                                          │                   ├─ 将 epitem 加入 rdllist
                                          │                   └─ 唤醒 epoll_wait 上的休眠进程

epoll_wait(epfd, events, max, timeout)
                                          ├─ 检查 rdllist:
                                          │    ├─ 非空 → 直接拷贝到 events 数组返回
                                          │    └─ 空 → schedule() 休眠
                                          │         └─ 某 fd 就绪 → ep_poll_callback()
                                          │              └─ 唤醒 → 拷贝 rdllist → 返回
                                          └─ copy_to_user(events)
```

### 4.4 事件类型

```c
struct epoll_event {
    uint32_t     events;    // 事件掩码
    epoll_data_t data;      // 用户数据（union）
};

typedef union epoll_data {
    void    *ptr;    // 指向任意用户数据
    int      fd;     // 文件描述符（最常用）
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
```

```c
// 常用事件
EPOLLIN      // 可读（同 POLLIN）
EPOLLOUT     // 可写（同 POLLOUT）
EPOLLRDHUP   // 对方关闭连接（Linux 2.6.17+）
EPOLLPRI     // 带外数据
EPOLLERR     // 错误（自动设置）
EPOLLHUP     // 挂断（自动设置）
EPOLLET      // ★ 边缘触发（Edge-Triggered）— 关键
EPOLLONESHOT // 一次触发后自动删除，需重新 EPOLL_CTL_MOD
EPOLLEXCLUSIVE // 多 epoll 下只唤醒一个（见前文 7.2）
```

### 4.5 LT vs ET — 理解触发模式

这是 epoll 最核心也最容易搞混的概念。

```
Level-Triggered (LT，水平触发) — 默认模式：

数据到达 socket 缓冲区:
    ┌─────────────────────┐
    │ socket 缓冲区       │
    │ [H][e][l][l][o]     │  ← 有 5 字节数据
    └─────────────────────┘

epoll_wait 返回 → 告诉你有数据
    ↓ 你只 read() 了 2 字节
    ↓
    ┌─────────────────────┐
    │ [l][l][o]           │  ← 还剩 3 字节
    └─────────────────────┘

下次 epoll_wait → 又告诉你还有数据
    ↓ 你不读它就一直通知
    ↓
★ 特性：只要缓冲区还有数据，epoll_wait 就反复通知
★ 优点：不易漏事件，适合一次性 read/write
★ 缺点：可能重复通知同一个 fd

────────────────────────────────────────────────

Edge-Triggered (ET，边缘触发) — 设置 EPOLLET：

数据到达 socket 缓冲区:
    ┌─────────────────────┐
    │ socket 缓冲区       │
    │ [H][e][l][l][o]     │  ← 有 5 字节数据 (从空→非空的"边缘")
    └─────────────────────┘

epoll_wait 返回 → 告诉你有数据（这是第一次，也是唯一一次通知）
    ↓ 你只 read() 了 2 字节
    ↓
    ┌─────────────────────┐
    │ [l][l][o]           │  ← 还剩 3 字节
    └─────────────────────┘

下次 epoll_wait → 不再通知你!!!!!!
    ↓ 除非：
    │  1. 又有新数据到达（从非空→非空不算，必须从空→非空）
    │  2. 对方发了新数据 [W][o][r][l][d]（产生了新的"边缘"）
    │
★ 特性：只在状态发生变化时通知（空→非空，非空→空）
★ 缺点：必须一次把数据读完！否则永远丢失剩余数据
★ 强制要求：socket 必须设置为非阻塞，用 while 循环读到 EAGAIN
```

### 4.6 ET 模式的标准写法

```c
/* ★ ET 模式黄金法则：非阻塞 + while 循环 + EAGAIN 判断 */

// 设置非阻塞
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// 注册 epoll，使用 EPOLLET
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;      // ★ ET 边缘触发
ev.data.fd = fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

// 事件循环中处理可读
if (events[i].events & EPOLLIN) {
    while (1) {                         // ★ 必须 while 循环读完
        char buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            // 处理数据
        } else if (n == 0) {
            // 对方关闭
            close(fd);
            break;
        } else {
            if (errno == EAGAIN)        // ★ 数据读完，正确退出
                break;
            // 其他错误
            break;
        }
    }
}
```

**为什么必须非阻塞 + EAGAIN？**

```c
// LT 模式可以这样写（每次读一点，下次还会通知）:
n = read(fd, buf, 100);     // 读 100 字节，下次 epoll_wait 继续通知

// ET 模式这样写会丢失数据:
n = read(fd, buf, 100);     // 读 100 字节
// 如果还剩 900 字节，epoll_wait 不会再通知！！！
// 除非有新的数据到达
```

### 4.7 epoll 的优点总结

| 维度 | select | poll | epoll |
|------|--------|------|-------|
| **时间复杂度** | O(n) | O(n) | **O(1)**（就绪 fd 数量） |
| **fd 上限** | 1024 | 无 | 无（受 `max_user_watches` 限制） |
| **数据拷贝** | 全量拷贝（每次） | 全量拷贝（每次） | **只拷贝就绪的 fd** |
| **注册/监听分离** | ❌ | ❌ | ✅ `epoll_ctl` 注册，`epoll_wait` 监听 |
| **触发模式** | 只有水平 | 只有水平 | **水平 + 边缘** |

---

## 6. select / poll / epoll 深度对比

### 5.1 性能曲线

```
就绪 fd 数量固定为 100 时，总 fd 数变化的影响：

吞吐量
  ↑
  │
  │  epoll ──────────────────────────────────
  │
  │               poll ──────
  │                         
  │  select ────────────────
  │
  └──────────────────────────────────→ 总 fd 数量
     1k      10k     100k

★ epoll 几乎不受总 fd 数量影响
★ select/poll 随总 fd 增长线性下降
```

### 5.2 各场景的选型建议

| 场景 | 推荐方案 | 原因 |
|------|---------|------|
| **桌面应用**（≤ 100 fd） | `select` 或 `poll` | 简单够用，可移植性好 |
| **嵌入式 / 跨平台** | `poll` | 无 1024 限制，POSIX 标准 |
| **Linux 高并发服务器**（≥ 1000 fd） | `epoll` | O(1) 性能，ET 模式 |
| **macOS / FreeBSD** | `kqueue`（epoll 等价物） | epoll 是 Linux 专有 |
| **Windows** | `IOCP`（异步 I/O） | Windows 的异步模型 |
| **跨平台高性能需求** | `libuv` / `libevent` | 封装了各平台最优方案 |

### 5.3 epoll 的适用边界

epoll 并非在所有场景都比 poll 快：

```
如果总 fd 数量很少（< 100），且就绪比例很高（> 50%）:
  poll 可能比 epoll 快
  原因: epoll 每次需要 epoll_ctl 注册 + epoll_wait 两次系统调用
        poll 一次系统调用即可

如果连接非常活跃（每次 epoll_wait 返回大量就绪 fd）:
  epoll 的优势仍然明显（O(1) 不遍历不活跃的 fd）
```

---

## 7. 非阻塞 I/O

### 6.1 为什么需要非阻塞

在 epoll ET 模式下，**socket 必须是非阻塞的**（原因见 4.6 节）。此外，非阻塞 I/O 还有两个典型场景：

1. **connect 非阻塞**：检测连接是否完成（非阻塞 connect + epollout）
2. **accept 非阻塞**：用 while 循环一次 accept 完所有已就绪的连接

### 6.2 设置非阻塞

```c
// 方法一：fcntl（设置已有 fd）
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// 方法二：socket 时直接指定（推荐，原子操作）
int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

// 方法三：accept 时直接继承（Linux 2.6.31+）
int conn = accept4(listen_fd, &addr, &len, SOCK_NONBLOCK);
```

### 6.3 非阻塞 connect

```c
int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
struct sockaddr_in addr = { ... };

int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
if (ret < 0) {
    if (errno == EINPROGRESS) {
        // ★ connect 正在进行中（不会阻塞）
        // 用 epoll/epoll 监听这个 fd 的 EPOLLOUT 事件
        // 当 EPOLLOUT 触发时，连接建立完成
        struct epoll_event ev = {.events = EPOLLOUT, .data.fd = fd};
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    } else {
        perror("connect");
    }
}

// 事件循环中：
if (events[i].events & EPOLLOUT) {
    int err;
    socklen_t len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err == 0) {
        printf("Async connect succeeded!\n");
    } else {
        printf("Async connect failed: %s\n", strerror(err));
    }
}
```

### 6.4 非阻塞 accept

适用于 epoll 检测到 listen_fd 可读时：

```c
// 先把 listen_fd 设为非阻塞
int flags = fcntl(listen_fd, F_GETFL, 0);
fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

// epoll 通知 listen_fd 可读时
while (1) {                             // ★ while 循环
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int conn = accept(listen_fd, (struct sockaddr*)&addr, &len);
    if (conn >= 0) {
        // 处理新连接
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK)  // ★ 没有更多连接了
            break;
        if (errno == ECONNABORTED)      // 对方放弃连接
            continue;
        if (errno == EINTR)             // 信号中断
            continue;
        break;
    }
}
```

---

## 8. 高级 I/O 函数

### 7.1 `readv()` / `writev()` — 分散/聚集 I/O

一次系统调用读写多个**不连续**的缓冲区，减少系统调用次数。

```c
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

struct iovec {
    void  *iov_base;    // 缓冲区起始地址
    size_t iov_len;     // 缓冲区长度
};
```

**示例：用 writev 发送 HTTP 响应（避免数据拷贝）**

```c
// 传统做法：先拼接再发送
char response[4096];
int len = snprintf(response, sizeof(response),
    "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\n\r\n%s",
    body_len, body);
write(fd, response, len);
/* 问题：snprintf 多了一次内存拷贝 */

// writev 做法：直接发送三个分离的缓冲区
struct iovec iov[3] = {
    {.iov_base = "HTTP/1.1 200 OK\r\n",  .iov_len = 17},
    {.iov_base = header_buf,              .iov_len = header_len},
    {.iov_base = body,                    .iov_len = body_len},
};
writev(fd, iov, 3);
/* 优点：零拷贝，不需要拼接到中间缓冲区 */
```

**内核视角**：

```
writev(fd, iov, 3)
  │
  └─ sys_writev()
       └─ do_readv_writev()
            ├─ 遍历 iov[0..2]
            │    for 每个 iovec:
            │        import_single_range()  ← 把用户态地址映射到内核
            │        sock->ops->sendmsg()   ← 通过同一个 socket 发送
            └─ 返回总写入字节数
```

### 7.2 `sendmsg()` / `recvmsg()` — 最强的 I/O 函数

这两个函数是**最通用的 I/O 函数**，能完成其他所有 I/O 函数的功能，还能传递辅助数据。

```c
#include <sys/socket.h>

ssize_t sendmsg(int fd, const struct msghdr *msg, int flags);
ssize_t recvmsg(int fd, struct msghdr *msg, int flags);

struct msghdr {
    void         *msg_name;       // 对方地址（UDP 用，TCP 可 NULL）
    socklen_t     msg_namelen;    // 地址长度
    struct iovec *msg_iov;        // 数据缓冲数组（scatter/gather）
    size_t        msg_iovlen;     // iov 数组长度
    void         *msg_control;    // ★ 辅助数据（传递 fd、证书等）
    size_t        msg_controllen; // 辅助数据长度
    int           msg_flags;      // 接收时返回的标志
};
```

**核心用途**：

| 功能 | 实现方式 | 替代者 |
|------|---------|--------|
| 普通读写 | `msg_iov` | `read/write` |
| UDP 收发 | `msg_name` 指定地址 | `sendto/recvfrom` |
| 分散/聚集 | `msg_iov` 多段缓冲 | `readv/writev` |
| **传递文件描述符** | `msg_control` + `SCM_RIGHTS` | **仅 sendmsg** |
| **传递凭据** | `msg_control` + `SCM_CREDENTIALS` | **仅 sendmsg** |

**传递文件描述符（fd 传递）的高级用法**：

```c
/* 通过 Unix Domain Socket 把 conn_fd 发给另一个进程 */
void send_fd(int unix_sock, int fd_to_send) {
    struct msghdr msg = {0};
    char buf[] = "x";           // 至少发 1 字节数据（虽然 unused）
    struct iovec iov = {.iov_base = buf, .iov_len = 1};

    /* 辅助数据缓冲区 */
    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;           // ★ 传递文件描述符
    *(int*)CMSG_DATA(cmptr) = fd_to_send;    // ★ 放入要传递的 fd

    sendmsg(unix_sock, &msg, 0);
}
```

### 7.3 `splice()` — 零拷贝数据管道

```c
#define _GNU_SOURCE
#include <fcntl.h>

ssize_t splice(
    int fd_in,  loff_t *off_in,  // 源 fd（必须一个是管道）
    int fd_out, loff_t *off_out, // 目标 fd
    size_t len,                  // 要移动的字节数
    unsigned int flags           // SPLICE_F_MOVE / SPLICE_F_NONBLOCK / SPLICE_F_MORE
);
```

**作用**：在两个文件描述符之间**移动数据，不经过用户态缓冲区**（零拷贝）。

```
普通 I/O:   磁盘 → [内核缓冲区] → [用户缓冲区] → [内核缓冲区] → socket
                                     ↑ 两次拷贝

splice:     磁盘 → [内核缓冲区] ──────────────────→ socket
                                     ↑ 零拷贝（直接移动页）
```

**示例：零拷贝静态文件服务器**

```c
// 用 splice 发送静态文件，无需分配用户态缓冲区
int file_fd = open("index.html", O_RDONLY);
int pipe_fds[2];
pipe(pipe_fds);             // splice 要求至少一端是管道

while (1) {
    // 文件 → 管道
    ssize_t n = splice(file_fd, NULL, pipe_fds[1], NULL, 4096, 0);
    if (n <= 0) break;

    // 管道 → socket
    splice(pipe_fds[0], NULL, conn_fd, NULL, n, 0);
}

close(file_fd);
close(pipe_fds[0]);
close(pipe_fds[1]);
```

### 7.4 `sendfile()` — 文件到 socket 的零拷贝

```c
#include <sys/sendfile.h>

ssize_t sendfile(
    int out_fd,    // 目标 fd（必须是 socket）
    int in_fd,     // 源 fd（必须是文件，mmap 可用）
    off_t *offset, // 文件偏移（NULL = 从当前位置）
    size_t count   // 要发送的字节数
);
```

**相比 splice 的简化**：不需要管道作为中介，直接文件→socket。

```c
int file_fd = open("index.html", O_RDONLY);
off_t offset = 0;
size_t remaining = file_size;

while (remaining > 0) {
    ssize_t n = sendfile(conn_fd, file_fd, &offset, remaining);
    if (n <= 0) {
        if (errno == EAGAIN) continue;  // 非阻塞，重试
        break;
    }
    remaining -= n;
}
```

### 7.5 `recv()` / `send()` 的标志位

```c
ssize_t recv(int fd, void *buf, size_t len, int flags);
ssize_t send(int fd, const void *buf, size_t len, int flags);
// flags=0 等价于 read/write
```

**常用 flags**：

| 标志 | 作用 | 场景 |
|------|------|------|
| `MSG_PEEK` | **偷看**数据不取出（数据留在缓冲区） | 预检协议头 |
| `MSG_DONTWAIT` | 非阻塞，等价于 O_NONBLOCK | 临时非阻塞读取 |
| `MSG_WAITALL` | 等待指定字节数全部到达才返回 | 固定长度协议包 |
| `MSG_NOSIGNAL` | 不触发 SIGPIPE，返回 EPIPE | 类似忽略 SIGPIPE |
| `MSG_OOB` | 接收带外数据 | 紧急数据 |

**MSG_PEEK 示例**：

```c
char header[4];
// 先偷看 4 字节，不取出 — 数据仍然在缓冲区中
ssize_t n = recv(fd, header, 4, MSG_PEEK);
if (n >= 4) {
    uint32_t body_len = ntohl(*(uint32_t*)header);  // 解析长度
    // 确认了整个包的长度后再实际读取
    char *buf = malloc(body_len);
    recv(fd, buf, body_len, MSG_WAITALL);  // 真正读取
}
```

---

## 9. 信号驱动 I/O（SIGIO）

### 8.1 基本原理

信号驱动 I/O 是 Unix 第四种 I/O 模型：进程告诉内核 "当 fd 就绪时发 SIGIO 信号通知我"，然后进程可以去做其他事。

```
进程:                          内核:
  │                              │
  │ fcntl(fd, F_SETOWN, pid)     │  设置 fd 的属主进程
  │ fcntl(fd, F_SETFL, O_ASYNC)  │  开启异步通知
  │─────────────────────────────>│
  │                              │
  │ 做其他事 (不阻塞)             │
  │                              │
  │                       ← 数据到达
  │                              │
  │ ← SIGIO 信号                │  内核发信号通知
  │                              │
  │ 信号处理函数:
  │   recv(fd, buf, len, 0)     │  读取数据
```

### 8.2 为什么信号驱动 I/O 在网络编程中几乎被弃用

| 问题 | 原因 |
|------|------|
| **信号丢失** | 标准信号不排队，多个 SIGIO 可能合并为一个 |
| **信号安全限制** | 信号处理函数中只能调用 async-signal-safe 的函数 |
| **UDP 问题** | 无法知道数据报的来源地址 |
| **性能** | 高频率信号 → 信号处理开销 > epoll 轮询开销 |
| **可移植性** | 不同 Unix 实现差异大 |

**结论**：信号驱动 I/O 在**网络编程中已被 epoll 完全取代**（但在**终端 I/O 和某些特殊 fd** 中仍有使用）。

---

## 10. 多播（Multicast）

### 9.1 什么是多播

多播是一对多的通信方式：**一个发送者，多个接收者（属于同一个组）**。

```
单播 (Unicast):             广播 (Broadcast):           多播 (Multicast):
  发送者                     发送者                        发送者
    │                          │                             │
    ├──→ 接收者 A              ├──→ 所有人                   ├──→ 接收者 A
    ├──→ 接收者 B              ├──→ 所有人                   ├──→ 接收者 B
    └──→ 接收者 C              ├──→ 所有人                   │ 接收者 C（不在组内）
                                                             │
                            ★ 广播：发给子网内所有机器        ★ 多播：只发给加组的机器
                            ★ 路由器默认不转发广播            ★ 路由器可转发（IGMP）
                            ★ 所有机器都要处理（即使不想要）    ★ 不感兴趣的机器不受影响
```

### 9.2 IP 多播地址

IPv4 多播地址范围：**224.0.0.0 ~ 239.255.255.255**（D 类地址）

| 范围 | 用途 | 说明 |
|------|------|------|
| 224.0.0.0 – 224.0.0.255 | 链路本地（不转发） | 路由协议使用（OSPF、IGMP） |
| 224.0.1.0 – 238.255.255.255 | 全球范围 | 需要 IGMP 管理 |
| 239.0.0.0 – 239.255.255.255 | 管理范围 | 组织内部使用 |

**常用多播地址**：
- `224.0.0.1` — 所有多播主机
- `224.0.0.2` — 所有多播路由器
- `224.0.0.251` — mDNS（5353 端口）
- `239.255.255.250` — SSDP（简单服务发现协议）

### 9.3 多播的 socket 编程

#### 发送端（UDP + 多播地址）

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MULTICAST_ADDR "239.0.0.1"
#define PORT 8888

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* 设置多播 TTL（生存时间，限制传播范围） */
    int ttl = 16;
    setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    /* ★ 可选：禁用多播回环（发送者自己是否也接收）*/
    int loop = 0;
    setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr(MULTICAST_ADDR),
        .sin_port = htons(PORT),
    };

    char msg[] = "Hello, multicast group!";
    while (1) {
        sendto(fd, msg, strlen(msg), 0,
               (struct sockaddr*)&addr, sizeof(addr));
        printf("Sent: %s\n", msg);
        sleep(1);
    }
}
```

#### 接收端（加入多播组）

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MULTICAST_ADDR "239.0.0.1"
#define PORT 8888

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* 必须 bind 到多播端口 */
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,        // ★ 必须 INADDR_ANY
        .sin_port = htons(PORT),
    };
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    /* ★ 关键：加入多播组 */
    struct ip_mreq mreq = {
        .imr_multiaddr = { .s_addr = inet_addr(MULTICAST_ADDR) },
        .imr_interface  = { .s_addr = htonl(INADDR_ANY) },
    };
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    char buf[1024];
    while (1) {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        ssize_t n = recvfrom(fd, buf, sizeof(buf), 0,
                             (struct sockaddr*)&from, &from_len);
        if (n > 0) {
            buf[n] = '\0';
            printf("Received from %s: %s\n",
                   inet_ntoa(from.sin_addr), buf);
        }
    }
}
```

### 9.4 多播的内核实现

```
发送端:
  sendto(多播地址 239.0.0.1)
    │
    └─ IP 层: ip_append_data()
         ├─ 查询路由表 → 发现是多播地址
         ├─ 复制数据包（每个出接口一份）
         ├─ 递减 TTL
         └─ 发送到数据链路层

接收端:
  setsockopt(IP_ADD_MEMBERSHIP)
    │
    └─ 内核:
         ├─ 将 (多播组, 接口) 加入 inet->mc_list
         ├─ 通知网卡驱动更新多播过滤表
         └─ 网卡硬件过滤 or 软件过滤

  数据包到达网卡:
    └─ 网卡检查目的 MAC 是否为多播地址
         ├─ 硬件过滤: 检查多播过滤表（hash 表）
         │    ├─ 命中 → DMA 到内核缓冲区
         │    └─ 未命中 → 丢弃
         └─ 软件过滤: 内核检查 IP 多播组列表
              ├─ 有进程加入 → 递交给 socket
              └─ 无进程加入 → 丢弃
```

### 9.5 多播的 setsockopt 选项

| 选项 | 级别 | 作用 | 适用 |
|------|------|------|------|
| `IP_MULTICAST_TTL` | IPPROTO_IP | 设置 TTL（默认 1，限制传播范围） | 发送端 |
| `IP_MULTICAST_IF` | IPPROTO_IP | 指定发送多播的网卡 | 发送端 |
| `IP_MULTICAST_LOOP` | IPPROTO_IP | 是否回环给自己（0=不接收） | 发送端 |
| `IP_ADD_MEMBERSHIP` | IPPROTO_IP | **加入多播组** | 接收端 |
| `IP_DROP_MEMBERSHIP` | IPPROTO_IP | **离开多播组** | 接收端 |
| `IPV6_JOIN_GROUP` | IPPROTO_IPV6 | IPv6 加入多播组 | 接收端 |

### 9.6 `ip_mreq` 结构

```c
struct ip_mreq {
    struct in_addr imr_multiaddr;  // 多播组 IP（如 239.0.0.1）
    struct in_addr imr_interface;  // 本地网卡 IP（INADDR_ANY 表示任意网卡）
};
```

---

## 11. 广播（Broadcast）

### 10.1 什么是广播

广播是**一对所有**的通信方式：发送给**子网内所有机器**。

```
广播:
    ┌── 发送者
    │
    ├──→ 机器 A（处理）
    ├──→ 机器 B（处理）
    ├──→ 机器 C（处理）
    ├──→ 机器 D（处理）
    ├──→ 机器 E（处理）
    │
    ★ 发给子网内所有主机
    ★ 路由器默认不转发广播（防止广播风暴）
    ★ 所有主机必须处理（即使不感兴趣的应用程序）
```

### 10.2 广播地址

IPv4 广播地址：**网络号部分不变，主机号全 1**。

```
IP:      192.168.1.0/24
           └── 网络号 192.168.1
           └── 主机号 0

广播地址: 192.168.1.255     ← 主机号全 1

有限广播: 255.255.255.255   ← 只在本网段内广播
```

### 10.3 广播的 socket 编程

#### 发送端

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BROADCAST_ADDR "192.168.1.255"
#define PORT 9999

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* ★ 关键：必须设置 SO_BROADCAST 才能发送广播 */
    int broadcast = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr(BROADCAST_ADDR),
        .sin_port = htons(PORT),
    };

    char msg[] = "Broadcast message!";
    while (1) {
        sendto(fd, msg, strlen(msg), 0,
               (struct sockaddr*)&addr, sizeof(addr));
        sleep(5);
    }
}
```

#### 接收端

```c
int fd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(PORT),
};
bind(fd, (struct sockaddr*)&addr, sizeof(addr));
/* ★ 接收广播不需要额外 setsockopt（默认接收广播）*/

char buf[1024];
struct sockaddr_in from;
socklen_t from_len = sizeof(from);
recvfrom(fd, buf, sizeof(buf), 0,
         (struct sockaddr*)&from, &from_len);
```

### 10.4 广播 vs 多播

| 维度 | 广播 | 多播 |
|------|------|------|
| **通信范围** | **子网内所有机器** | **子网内/跨子网（IGMP）** |
| **路由器转发** | ❌ 不转发 | ✅ 可转发（需 IGMP 协议支持） |
| **性能影响** | 所有机器中断处理（即使不关心） | 只影响加入组的机器 |
| **IP 地址** | 子网广播地址（如 192.168.1.255） | D 类地址（224.0.0.0 – 239.255.255.255） |
| **setsockopt** | `SO_BROADCAST`（发送端必须） | `IP_ADD_MEMBERSHIP`（接收端必须） |
| **UDP 端口** | 接收端 bind 到端口即可 | 接收端加入组 + bind 端口 |
| **典型应用** | DHCP、ARP、NetBIOS | 视频直播、证券行情、服务发现 |

**从业界的实际使用趋势**：

```
1990s:       广播盛行（DHCP、ARP、RIP v1）
2000s:       多播开始替代广播（视频流、金融数据）
2010s+:      多播 + IGMP 成为标准（IPTV、证券行情）
现代倾向：     尽量用多播代替广播，减少不必要的网络开销
```

---

## 12. 基于 epoll 的并发 echo 服务器

综合运用本章知识，实现一个**单线程 + epoll ET + 非阻塞 I/O** 的并发 echo 服务器。

```c
// server_epoll_echo.c
// 编译: gcc -o server_epoll server_epoll_echo.c
// 测试: 多个终端同时 nc localhost 8080

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 64
#define BUF_SIZE 4096

/* 设置非阻塞 */
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    /* ─── 1. 创建监听 socket ─── */
    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT),
    };
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, SOMAXCONN);

    /* ─── 2. 创建 epoll 实例 ─── */
    int epfd = epoll_create1(0);

    /* ─── 3. 注册 listen_fd（LT 模式）─ */
    struct epoll_event ev = {
        .events = EPOLLIN,           // LT 水平触发（默认）— accept 适合 LT
        .data.fd = listen_fd,
    };
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    struct epoll_event events[MAX_EVENTS];
    printf("epoll echo server on port %d\n", PORT);

    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                /* ─── 新连接 ─── */
                while (1) {
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int conn = accept(listen_fd,
                                      (struct sockaddr*)&client, &len);
                    if (conn < 0) {
                        if (errno == EAGAIN) break;  // 全 accept 完了
                        if (errno == ECONNABORTED) continue;
                        break;
                    }

                    /* ★ 新连接设为非阻塞 + ET 模式 */
                    set_nonblocking(conn);
                    struct epoll_event cev = {
                        .events = EPOLLIN | EPOLLET,  // ET 边缘触发
                        .data.fd = conn,
                    };
                    epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &cev);
                    printf("New client: fd=%d\n", conn);
                }

            } else if (events[i].events & EPOLLIN) {
                /* ─── 客户端有数据可读 ─── */
                int fd = events[i].data.fd;
                char buf[BUF_SIZE];

                /* ★ ET 模式：必须 while 循环读到 EAGAIN */
                while (1) {
                    ssize_t n = read(fd, buf, sizeof(buf));
                    if (n > 0) {
                        write(fd, buf, n);   // echo
                    } else if (n == 0) {
                        /* 对方关闭 */
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        printf("Client fd=%d disconnected\n", fd);
                        break;
                    } else {
                        if (errno == EAGAIN) break;  // 数据读完
                        /* 错误 */
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        break;
                    }
                }
            }
        }
    }

    close(epfd);
    close(listen_fd);
    return 0;
}
```

**关键点总结**：

| 组件 | 设置 | 原因 |
|------|------|------|
| listen_fd | SOCK_NONBLOCK + **LT** 模式 | accept 用 LT 更简单，while 循环到 EAGAIN |
| 客户端 fd | set_nonblock + **ET** 模式 | ET 高效，但必须 while 读完 |
| 读数据 | while read() 到 EAGAIN | ET 模式下漏读数据将永久丢失 |
| accept | while accept() 到 EAGAIN | 一次 EPOLLIN 事件可能对应多个连接 |

---

## 13. 常见陷阱与最佳实践

### 12.1 epoll 的 `EPOLLIN` 与对方关闭

```c
// ❌ 错误：只检查 EPOLLIN，不检查 EPOLLRDHUP
if (events[i].events & EPOLLIN) {
    read(fd, buf, sizeof(buf));   // 如果对方已经 close，read 返回 0
    // → 忘记 close(fd) 和 epoll_ctl DEL → fd 泄漏
}

// ✅ 正确：优先检查关闭
if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
    close(fd);
    continue;
}
if (events[i].events & EPOLLIN) {
    // 正常读数据
}
```

### 12.2 `send()` / `write()` 的 EAGAIN

```c
// epoll ET 模式下，写也可能 EAGAIN（发送缓冲区满）
ssize_t n = write(fd, buf, len);
if (n < 0) {
    if (errno == EAGAIN) {
        // ★ 发送缓冲区满，不能再写了
        // 需要等 EPOLLOUT 事件触发后才能继续
        // 做法：把未写完的数据缓存到应用层缓冲区
        // 并注册 EPOLLOUT 事件
        append_to_send_queue(fd, buf, len);
        struct epoll_event ev = {
            .events = EPOLLIN | EPOLLOUT | EPOLLET,
            .data.fd = fd,
        };
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}

// EPOLLOUT 触发时，从队列取数据继续发送
if (events[i].events & EPOLLOUT) {
    send_queued_data(fd);          // 发送缓存中的数据
    if (queue_empty(fd)) {
        struct epoll_event ev = {
            .events = EPOLLIN | EPOLLET,   // 去掉 EPOLLOUT
            .data.fd = fd,
        };
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}
```

### 12.3 `listen_fd` 不应用 ET

```c
// ❌ 不推荐
struct epoll_event ev = {
    .events = EPOLLIN | EPOLLET,   // listen_fd 用 ET
    .data.fd = listen_fd,
};

// ✅ 推荐：listen_fd 用 LT + 非阻塞 accept
struct epoll_event ev = {
    .events = EPOLLIN,             // LT 足够
    .data.fd = listen_fd,
};
// accept 时用 while 循环到 EAGAIN 即可
```

### 12.4 epoll 的 `max_user_watches`

```c
// 查看当前限制
cat /proc/sys/fs/epoll/max_user_watches

// 每个被 epoll_ctl ADD 的 fd 在内核中占用约 90 字节
// 100 万连接需要约 90 MB 内核内存
// 如果不足会返回 ENOMEM
```

### 12.5 select/poll 必须注意的点

```c
// select：nfds 必须是最大 fd + 1
select(max_fd + 1, &readfds, NULL, NULL, NULL);
// 如果 max_fd=5，但 FD_SET(1000)，select 会... 嗯，它不会检查到 1000
// 因为 nfds=6，只检查 fd 0~5 → fd 1000 永远不会被 select 监视！

// 正确做法：始终用实际的 max_fd + 1
int max_fd = listen_fd;
for (每个 fd) {
    if (fd > max_fd) max_fd = fd;
}
select(max_fd + 1, ...);
```

### 12.6 多播的常见问题

```c
// ❌ 错误：bind 到多播地址
struct sockaddr_in addr = {
    .sin_addr.s_addr = inet_addr("239.0.0.1"),  // bind 到多播地址
};
bind(fd, ...);  // 可能失败或行为不一致

// ✅ 正确：bind 到 INADDR_ANY + 端口
struct sockaddr_in addr = {
    .sin_addr.s_addr = INADDR_ANY,              // 必须 INADDR_ANY
    .sin_port = htons(PORT),
};
bind(fd, ...);

// 然后加入多播组
struct ip_mreq mreq = {
    .imr_multiaddr.s_addr = inet_addr("239.0.0.1"),
    .imr_interface.s_addr = htonl(INADDR_ANY),
};
setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
```

### 12.7 广播的常见问题

```c
// ❌ 错误：忘记设置 SO_BROADCAST
sendto(fd, buf, len, 0, &broadcast_addr, sizeof(broadcast_addr));
// → 返回 EACCES（权限被拒）

// ✅ 正确
int on = 1;
setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
```

---

## 14. 附录：关键系统调用速查

### 13.1 I/O 多路复用

| 系统调用 | 平台 | 复杂度 | 上限 | 特点 |
|----------|------|--------|------|------|
| `select()` | 全平台 | O(n) | 1024 | 最可移植但最受限 |
| `poll()` | POSIX | O(n) | 无 | 无 1024 限制 |
| `epoll()` | Linux | O(1) | 无 | ET/LT、高性能 |
| `kqueue()` | BSD/macOS | O(1) | 无 | macOS/FreeBSD 的 epoll |
| `eventfd()` | Linux | — | — | 事件通知 fd（配合 epoll） |
| `timerfd()` | Linux | — | — | 定时器 fd（配合 epoll） |
| `signalfd()` | Linux | — | — | 信号转 fd（配合 epoll） |

### 13.2 高级 I/O

| 系统调用 | 作用 | 关键特性 |
|----------|------|---------|
| `readv()` / `writev()` | 分散/聚集 I/O | 一次调用读写多个不连续缓冲区 |
| `sendmsg()` / `recvmsg()` | 最通用的 I/O | 可以传递文件描述符、辅助数据 |
| `splice()` | 零拷贝管道移动 | 不经过用户态，需管道做中介 |
| `sendfile()` | 文件→socket 零拷贝 | 比 splice 简单，不需管道 |
| `tee()` | 管道间数据拷贝 | 不消耗数据（类似 peek） |
| `vmsplice()` | 用户内存→管道 | 用户页直接映射到管道 |

### 13.3 多播/广播选项

| 选项 | 系统调用 | 作用 |
|------|---------|------|
| `SO_BROADCAST` | `setsockopt` | 允许发送广播数据包 |
| `IP_ADD_MEMBERSHIP` | `setsockopt` | 加入多播组（接收） |
| `IP_DROP_MEMBERSHIP` | `setsockopt` | 离开多播组 |
| `IP_MULTICAST_TTL` | `setsockopt` | 设置多播 TTL |
| `IP_MULTICAST_IF` | `setsockopt` | 指定多播发送网卡 |
| `IP_MULTICAST_LOOP` | `setsockopt` | 多播回环控制 |

### 13.4 非阻塞与 fd 控制

| 系统调用 | 作用 |
|----------|------|
| `fcntl(fd, F_GETFL/F_SETFL, O_NONBLOCK)` | 设置非阻塞 |
| `accept4()` | accept + 同时设置标志（SOCK_NONBLOCK + SOCK_CLOEXEC）|
| `pipe2()` | pipe + 同时设置标志 |
| `dup3()` | dup + 同时设置标志 |

---

## 总结

I/O 多路复用是现代高并发网络服务器的核心技术。从 select 到 epoll 的演进，本质是**从"全量遍历"到"回调就绪"** 的思维转变：

```
select / poll:
  每次调用: "内核，帮我检查这 10000 个 fd 哪些就绪了"
  内核:      遍历这 10000 个 fd → 返回就绪的
  问题:      每次都遍历全部，O(n)

epoll:
  第一次:    "内核，帮我监控这 10000 个 fd（注册）"
  之后每次:  "内核，有就绪的吗？"
  内核:      只返回就绪的（从就绪链表取）
  优势:      只处理就绪的，O(1)
```

**选择指南**：

```
你的平台？
  ├─ Linux → epoll（首选，ET 模式更高效）
  ├─ macOS/FreeBSD → kqueue
  └─ 跨平台 → libevent / libuv

你的连接量？
  ├─ < 100 个连接 → select 或 poll（简单够用）
  ├─ 100 - 10000 → epoll / kqueue（性能无压力）
  └─ 10000+ → epoll ET + 非阻塞 I/O（最优）
```

**进一步学习方向**：
- [ ] 阅读内核源码：`fs/select.c`、`fs/eventpoll.c`、`net/ipv4/ip_sockglue.c`（多播）
- [ ] 实践：用 `strace` 观察 epoll 的系统调用
- [ ] 练习：实现一个 epoll ET + 非阻塞的 HTTP/1.1 服务器
- [ ] 进阶：对比 libevent 和 libuv 的事件循环实现
- [ ] 参考：UNIX Network Programming (Stevens)、Linux System Programming

---

> 📌 **本文档是对《多进程服务器端编程》的并行篇，两篇合起来覆盖了 Unix 网络编程多路并发、两种主流并发模型。**

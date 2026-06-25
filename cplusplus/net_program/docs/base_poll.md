# select、poll、epoll 详解

> 本文面向**基础薄弱**的读者，用白话 + 代码 + 对比的方式，把这三个 I/O 多路复用机制讲清楚。

---

## 目录

1. [前置知识：什么是 I/O 多路复用？](#1-前置知识什么是-io-多路复用)
2. [select —— 老前辈](#2-select--老前辈)
3. [poll —— 改进版 select](#3-poll--改进版-select)
4. [epoll —— Linux 上的王者](#4-epoll--linux-上的王者)
5. [三者对比总表](#5-三者对比总表)
6. [面试常见问题](#6-面试常见问题)

---

## 1. 前置知识：什么是 I/O 多路复用？

### 1.1 一个生活例子

想象你是一个前台接待员：

- **阻塞 I/O**：你只盯着一个窗口等一个人来办事，其他窗口来人了你也看不见 —— 效率极低。
- **多线程/多进程**：你招了 100 个接待员，每人盯一个窗口 —— 人多了，管理成本爆炸。
- **I/O 多路复用**：你还是一个人，但手里拿了一张**「通知单」**。谁来了，通知单就震动告诉你「3 号窗口有人了，快去处理」。你一个人就能管几百个窗口。

### 1.2 核心概念

I/O 多路复用（I/O Multiplexing）允许**一个进程**同时监控**多个文件描述符**（file descriptor, fd），当其中某个 fd 可读/可写/发生异常时，通知进程去处理。

> **文件描述符是什么？**
> 简单说，Linux 里一切都是文件。一个网络连接、一个管道、一个普通文件…… 内核都给分配一个整数编号，这个编号就是 fd。比如标准输入是 0，标准输出是 1，标准错误是 2。

### 1.3 为什么需要它？

```c
// 如果没有 I/O 多路复用，你要同时读两个 socket：
// 错误做法 —— 阻塞在第一个 read 上，第二个 socket 永远读不到
char buf1[1024], buf2[1024];
read(sock1, buf1, sizeof(buf1));  // 如果 sock1 没数据，卡死在这
read(sock2, buf2, sizeof(buf2));  // 永远执行不到

// 或者用非阻塞 + 忙等 —— CPU 空转，浪费
while (1) {
    int n1 = read(sock1, buf1, sizeof(buf1));  // 非阻塞
    int n2 = read(sock2, buf2, sizeof(buf2));  // 非阻塞
    // 都没数据就继续循环 —— CPU 100%
}
```

**I/O 多路复用解决的就是这个问题**：你告诉内核「帮我盯着这些 fd，哪个有动静了告诉我」，然后**阻塞等待**内核的通知，CPU 省下来做别的事。

---

## 2. select —— 老前辈

### 2.1 原型

```c
#include <sys/select.h>
#include <sys/time.h>

int select(int nfds,                     // 最大 fd 编号 + 1
           fd_set *readfds,              // 要监听可读的 fd 集合
           fd_set *writefds,             // 要监听可写的 fd 集合
           fd_set *exceptfds,            // 要监听异常的 fd 集合
           struct timeval *timeout);     // 超时时间

// 操作 fd_set 的宏
void FD_ZERO(fd_set *set);               // 清空集合
void FD_SET(int fd, fd_set *set);        // 把 fd 加入集合
void FD_CLR(int fd, fd_set *set);        // 把 fd 从集合移除
int  FD_ISSET(int fd, fd_set *set);      // 判断 fd 是否在集合中（返回后有事件）
```

### 2.2 怎么用 —— 一步一步来

```c
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int main() {
    fd_set read_fds;           // 1. 声明一个 fd 集合
    FD_ZERO(&read_fds);        // 2. 清空它
    FD_SET(0, &read_fds);      // 3. 把标准输入 (fd=0) 加进去

    // 4. 调用 select，只监听可读
    //    第一个参数传 1（最大 fd=0，所以 nfds = 0+1 = 1）
    int ret = select(1, &read_fds, NULL, NULL, NULL);
    // ^ 阻塞在这里，直到标准输入有数据可读

    if (ret == -1) {
        perror("select error");
    } else if (ret == 0) {
        printf("timeout\n");   // 这里 timeout 传 NULL，不会超时
    } else {
        // 判断是不是标准输入就绪了
        if (FD_ISSET(0, &read_fds)) {
            char buf[100];
            read(0, buf, sizeof(buf));
            printf("你输入了: %s", buf);
        }
    }
    return 0;
}
```

### 2.3 select 的优缺点

| 优点 | 缺点 |
|------|------|
| 几乎所有操作系统都支持（跨平台好） | `fd_set` 大小固定（默认 1024，即 FD_SETSIZE） |
| 接口简单，容易理解 | 每次调用都要把整个 `fd_set` **从用户态拷贝到内核态**，开销大 |
| 超时精度到微秒 | 内核会修改 `fd_set`（返回值只保留就绪的 fd），下次用**必须重新设置** |
| | 返回后需要**遍历所有 fd** 才能知道哪些就绪了 —— O(n) |
| | `nfds` 要传最大 fd+1，如果 fd 很大（比如 10086），浪费 |

### 2.4 一个更有实际意义的网络例子

```c
// 伪代码：用 select 管理多个 socket 连接
int server_fd = socket(...);   // 监听 socket
bind(server_fd, ...);
listen(server_fd, 5);

fd_set master_fds;             // 保存所有要监控的 fd
FD_ZERO(&master_fds);
FD_SET(server_fd, &master_fds);
int max_fd = server_fd;

while (1) {
    fd_set read_fds = master_fds;  // 必须拷贝一份！因为 select 会修改它

    int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

    for (int fd = 0; fd <= max_fd; fd++) {
        if (!FD_ISSET(fd, &read_fds)) continue;

        if (fd == server_fd) {
            // 有新连接
            int client_fd = accept(server_fd, ...);
            FD_SET(client_fd, &master_fds);
            if (client_fd > max_fd) max_fd = client_fd;
        } else {
            // 有客户端发数据
            char buf[1024];
            int n = read(fd, buf, sizeof(buf));
            if (n <= 0) {
                close(fd);
                FD_CLR(fd, &master_fds);   // 清理关闭的连接
            } else {
                // 处理数据...
            }
        }
    }
}
```

> **关键问题**：每次循环都要把 `master_fds` 拷贝到 `read_fds`（O(n) 拷贝），for 循环从 0 到 max_fd 遍历（O(n) 检查）。连接数一多（几千个），CPU 全耗在拷贝和遍历上了。

---

## 3. poll —— 改进版 select

### 3.1 原型

```c
#include <poll.h>

struct pollfd {
    int   fd;         // 要监控的文件描述符
    short events;     // 感兴趣的事件（输入参数）
    short revents;    // 实际发生的事件（输出参数）
};

int poll(struct pollfd *fds,     // pollfd 数组
         nfds_t nfds,            // 数组元素个数
         int timeout);           // 超时（毫秒），-1 = 永远等待
```

### 3.2 怎么用

```c
#include <stdio.h>
#include <poll.h>
#include <unistd.h>

int main() {
    struct pollfd fds[1];        // 1. 声明 pollfd 数组
    fds[0].fd = 0;               // 2. 监控标准输入
    fds[0].events = POLLIN;      // 3. 关注「可读」事件

    int ret = poll(fds, 1, -1);  // 4. 调用 poll，-1 表示永久等待

    if (ret == -1) {
        perror("poll error");
    } else if (ret == 0) {
        printf("timeout\n");
    } else {
        // 判断标准输入是否就绪
        if (fds[0].revents & POLLIN) {
            char buf[100];
            read(0, buf, sizeof(buf));
            printf("你输入了: %s", buf);
        }
    }
    return 0;
}
```

### 3.3 poll 比 select 好在哪里

| 改进点 | select | poll |
|--------|--------|------|
| **无上限** | `fd_set` 固定 1024 | 基于数组，只受内存限制 |
| **数据结构** | bitmap 位图（O(n) 拷贝） | 可变数组指针（仍然要拷贝） |
| **输入输出分离** | `fd_set` 被内核修改，下次要重设 | `events` 和 `revents` 分开，输入输出不冲突 |
| **遍历起点** | 要从 0 遍历到 max_fd | 遍历 fds 数组即可 |
| **超时精度** | 微秒 | 毫秒（反而下降了，但多数场景够用） |

### 3.4 poll 的网络例子

```c
// 伪代码
struct pollfd *fds = malloc(max_clients * sizeof(struct pollfd));
fds[0].fd = server_fd;
fds[0].events = POLLIN;
int nfds = 1;

while (1) {
    int ret = poll(fds, nfds, -1);

    for (int i = 0; i < nfds; i++) {
        if (!(fds[i].revents & POLLIN)) continue;

        if (fds[i].fd == server_fd) {
            int client_fd = accept(server_fd, ...);
            fds[nfds].fd = client_fd;
            fds[nfds].events = POLLIN;
            nfds++;
        } else {
            // 处理客户端数据...
        }
    }
}
```

### 3.5 poll 的缺点

1. **仍然要拷贝**：每次调用 `poll` 时，内核要把整个 `fds` 数组从**用户态拷贝到内核态**。几万个连接就是几万次拷贝。
2. **仍然要遍历**：内核遍历所有 fd 检查状态，返回后用户程序也要遍历所有 fd 看 `revents`。O(n) 逃不掉。
3. **水平触发**：如果某个 fd 的数据没读完，下次 `poll` 会再次通知 —— 如果忘记读，会一直通知（后面 epoll 有边沿触发）。

---

## 4. epoll —— Linux 上的王者

### 4.1 核心思想

select/poll 的问题是：
- 每次把**全部 fd** 传给内核 —— 像每年把全班同学叫到操场上点名
- 内核把**全部 fd** 检查一遍 —— 像老师挨个看谁在不在
- 回来告诉你「**有人到了**」 —— 但不说是谁，你得自己再挨个问

epoll 的做法是：
- 在内核里**建一个花名册** —— 注册一次就行
- 谁有动静了，内核**主动把就绪的 fd 记在另一张表上**
- 你只取走「**就绪列表**」 —— 不用逐个检查

> 一句话：**select/poll 说「有人到了，你自己找」，epoll 说「张三、李四到了，这是名单」。**

### 4.2 三个函数

```c
#include <sys/epoll.h>

// 1. 创建 epoll 实例 —— 在内核中建一个「花名册」
int epoll_create(int size);
// size 在 Linux 2.6.8 之后被忽略，但传 >0 的值
// 返回 epoll 文件描述符，后续操作用它

// 2. 控制花名册 —— 添加/修改/删除要监控的 fd
int epoll_ctl(int epfd,           // epoll 实例的 fd
              int op,             // 操作：EPOLL_CTL_ADD / MOD / DEL
              int fd,             // 要操作的目标 fd
              struct epoll_event *event);  // 关注的事件

// 3. 等待事件 —— 取出就绪的 fd
int epoll_wait(int epfd,
               struct epoll_event *events,  // 输出参数：就绪的事件数组
               int maxevents,               // events 数组大小
               int timeout);                // 超时（毫秒），-1 = 永久
```

### 4.3 结构体说明

```c
struct epoll_event {
    uint32_t     events;  // 感兴趣的事件（EPOLLIN/EPOLLOUT/EPOLLERR/EPOLLET...）
    epoll_data_t data;    // 用户数据联合体
};

typedef union epoll_data {
    void    *ptr;   // 可以绑定任意指针（比如一个结构体）
    int      fd;    // 最常用：直接存文件描述符
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
```

> `epoll_data_t` 是一个**联合体**（union），四个字段**共用一块内存**，一次只能存一个。最常用的是 `data.fd` —— 这样 `epoll_wait` 返回后直接用 `events[i].data.fd` 就知道是哪个 fd 就绪了。

### 4.4 怎么用 —— 最经典的模式

```c
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

int main() {
    // 1. 创建 epoll 实例
    int epfd = epoll_create(1);  // 1 是历史遗留，传 >0 即可

    // 2. 把标准输入加进去
    struct epoll_event ev;
    ev.events = EPOLLIN;        // 关注可读事件
    ev.data.fd = 0;             // 存下 fd
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);

    // 3. 准备一个就绪事件数组（epoll_wait 会往这里写）
    struct epoll_event ready_evs[10];

    while (1) {
        // 4. 等事件 —— 最多取 10 个就绪事件
        int n = epoll_wait(epfd, ready_evs, 10, -1);

        // 5. 直接遍历就绪数组 —— 不用全量扫描！
        for (int i = 0; i < n; i++) {
            if (ready_evs[i].data.fd == 0) {
                char buf[100];
                read(0, buf, sizeof(buf));
                printf("你输入了: %s", buf);
            }
        }
    }

    close(epfd);
    return 0;
}
```

### 4.5 触发模式：水平触发 vs 边沿触发

这是 select/poll 没有（或者说 select/poll 只有水平触发）而 epoll 独有的重要概念。

#### 水平触发（Level-Triggered, LT）—— 默认模式

> **只要还有数据没读完，epoll_wait 就一直通知你。**

```c
// 假设 fd 上有 100 字节数据
// 你只读了 30 字节
// 下次 epoll_wait —— 又通知你「还有数据」
// 你再读 30 字节
// 下次 epoll_wait —— 又通知……
// 直到全部读完
```

- **优点**：不容易丢事件，编程简单，不容易死锁
- **缺点**：可能重复通知，效率稍低

#### 边沿触发（Edge-Triggered, ET）—— 高效模式

> **只在状态变化（从无数据到有数据）的那一刻通知一次。之后无论还剩多少数据，都不会再通知。**

```c
// 使用方法：在 events 中加上 EPOLLET 标志
ev.events = EPOLLIN | EPOLLET;

// 假设 fd 上有 100 字节数据
// epoll_wait 通知你一次
// 你只读了 30 字节
// 下次 epoll_wait —— 不会通知你！
// 剩下的 70 字节就留在缓冲区里了
```

**ET 模式下必须这么做**：

```c
// 收到通知后，必须把数据全部读完！
// 用 while 循环非阻塞读：
while (1) {
    char buf[4096];
    int n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        // 处理数据
    } else if (n == -1 && errno == EAGAIN) {
        // 读完了！没有更多数据了，退出循环
        break;
    } else {
        // 真正的错误
        break;
    }
}
```

| | 水平触发 (LT) | 边沿触发 (ET) |
|--|--|--|
| 通知次数 | 只要有数据就通知 | 只在状态变化时通知一次 |
| 编程难度 | 简单 | 难（必须一次性读完，容易丢数据） |
| 性能 | 低（重复通知） | 高（通知次数少） |
| 适用场景 | 简单应用、新手 | 高性能服务器（Nginx、Redis 都用 ET） |

> **面试高频题**：Nginx 用 epoll 的 ET 模式还是 LT 模式？
> 答：ET 模式。Nginx 用 ET 配合非阻塞 I/O，实现高性能事件驱动。

### 4.6 epoll 高效的原因 —— 三个为什么

#### 为什么 epoll 比 select/poll 快 —— 事件回调机制

```text
select/poll 的工作方式：
用户态                 内核态
  │                     │
  ├── 传入所有 fd ─────→│
  │                     ├── 遍历所有 fd，检查状态
  │←──── 返回结果 ──────┤
  │                     │
  ├── 下次再传所有 fd ──→│  ← 每次都重新遍历！
  │                     │


epoll 的工作方式：
用户态                 内核态
  │                     │
  ├── epoll_ctl 注册 ──→│  ├── 为每个 fd 注册回调函数
  │                     │  │    （一次注册，永久生效）
  │                     │  │
  │←──── 返回 ─────────┤  │
  │                     │  │
  │                     │  └── 当 fd 有数据时，回调函数把 fd
  │                     │       放到「就绪链表」中
  │                     │
  ├── epoll_wait ──────→│
  │                     ├── 直接从就绪链表取数据
  │←── 就绪 fd 列表 ────┤      不用遍历所有 fd！
  │                     │
```

> **关键区别**：select/poll 是**主动轮询**（每次问「这个有数据吗？」那个呢？），epoll 是**事件驱动**（数据来了自动通知你），复杂度从 O(n) 降到 O(1)。

#### 为什么 epoll 拷贝少 —— mmap

select/poll 每次调用都要把 fd 集合从**用户态拷贝到内核态**（几十万次系统调用，每次拷贝几百 KB 的数据）。

epoll 在 `epoll_create` 时用 `mmap` 在内核和用户空间**共享一块内存**。注册 (`epoll_ctl`) 和等待 (`epoll_wait`) 时，双方直接读写共享内存，**不需要拷贝**。

> `mmap`（Memory-Mapped File）是一种内存映射技术，把内核空间的一块地址映射到用户空间，双方都能直接访问，省去了 `copy_from_user` / `copy_to_user` 的开销。

#### 为什么 epoll 取结果快 —— 只返回就绪的

- **select**：返回一个 bitmap，用户程序从 0 到 `max_fd` 逐个检查 `FD_ISSET` —— O(n)
- **poll**：返回一个数组，用户程序从 0 到 `nfds` 逐个检查 `revents` —— O(n)
- **epoll**：返回**只包含就绪 fd 的数组**，用户程序直接遍历 ready 数组 —— O(k)，k 是就绪连接数

> 当只有 10 个连接活跃时，select/poll 要遍历 10000 个，epoll 只遍历 10 个。

---

## 5. 三者对比总表

| 维度 | select | poll | epoll |
|------|--------|------|-------|
| **底层数据结构** | bitmap 位图 | pollfd 数组 | 红黑树 + 就绪链表 |
| **最大 fd 数量** | 1024（可改，但麻烦） | 无上限（受内存限制） | 无上限 |
| **fd 集合管理** | 每次调用都重新传入 | 每次调用都重新传入 | `epoll_ctl` 注册一次，永久生效 |
| **数据拷贝** | 每次调用从用户态拷贝到内核态 | 每次调用从用户态拷贝到内核态 | 通过 mmap 共享内存，减少拷贝 |
| **事件查找方式** | 遍历所有 fd（O(n)） | 遍历所有 fd（O(n)） | 直接读就绪链表（O(1)） |
| **触发模式** | 仅水平触发 | 仅水平触发 | 水平 + 边沿触发 |
| **返回结果** | 修改 fd_set，用户程序遍历 | 用户程序遍历 `revents` | 直接返回就绪事件数组 |
| **跨平台** | 几乎全部平台 | POSIX 标准 | **仅 Linux** |
| **性能（连接数少，如 <100）** | 还行 | 还行 | 也不错（但 setup 开销稍大） |
| **性能（连接数多，如 10000）** | 极差 | 差 | 好 |
| **性能（活跃连接少）** | 差（仍然遍历全部） | 差（仍然遍历全部） | **最好**（只返回活跃的） |
| **性能（活跃连接多）** | 差 | 差 | 好 |
| **编程复杂度** | 低 | 低 | 中（ET 模式更难） |

### 一句话总结

| 机制 | 一句话 |
|------|--------|
| **select** | 「我帮你盯着这 1024 个，有动静了告诉你 —— 但你自己找是哪个」 |
| **poll** | 「没有 1024 限制了，但你仍然要自己找是哪个」 |
| **epoll** | 「谁有动静我主动告诉你，你就处理这几个就行。而且一次注册，永久生效」 |

---

## 6. 面试常见问题

### Q1: select 为什么最多 1024？

`fd_set` 是用 bitmap 实现的，在 `<sys/select.h>` 中定义：

```c
#define FD_SETSIZE 1024
#define __NFDBITS (8 * (int) sizeof(__fd_mask))
typedef struct {
    __fd_mask fds_bits[FD_SETSIZE / __NFDBITS];
} fd_set;
```

这个 1024 是编译期常量，可以修改（重新定义 `FD_SETSIZE` 然后重新编译），但不推荐 —— 就算改大了，select 的 O(n) 拷贝和遍历问题依然存在。

### Q2: epoll 是线程安全的吗？

- `epoll_ctl` 是线程安全的（有内部锁保护红黑树）
- `epoll_wait` 也是线程安全的
- 但多个线程同时调用 `epoll_wait` 等待同一个 epoll fd 时，**多个线程可能都被唤醒处理同一个 fd**，需要额外同步

### Q3: 既然 epoll 这么好，为什么还需要 select/poll？

1. **跨平台**：macOS、Windows 不支持 epoll（macOS 用 kqueue，Windows 用 IOCP）
2. **连接数少时**：select/poll 简单高效，epoll 的 `epoll_ctl` 系统调用也有开销
3. **历史代码**：大量遗留代码用 select/poll
4. **epoll 仅限 Linux**：嵌入式或其他 Unix 系统只能用 select/poll

### Q4: Redis 为什么用 select/epoll 混合？Nginx 呢？

- **Redis**：单线程，连接数通常不多（几千），用 select 还是 epoll 差别不大。Redis 在编译时自动选择：有 epoll 用 epoll，没有用 select。
- **Nginx**：必须用 epoll（ET 模式），因为 Nginx 要处理几万甚至几十万并发连接，select/poll 完全扛不住。

### Q5: 实际开发中选哪个？

| 场景 | 推荐 |
|------|------|
| Linux 服务器，连接数 > 1000 | **epoll** |
| Linux 服务器，连接数 < 100 | 三者都行，poll 更简单 |
| 跨平台（macOS/Windows） | 用 libevent / libuv 等封装库，底层自动选择 |
| 写玩具/学习 | poll（简单且无 1024 限制） |
| 面试手写 | 准备 epoll 就足够 |

---

## 附：用 epoll 写一个简单的 TCP 服务器（完整代码）

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>

#define PORT 8888
#define MAX_EVENTS 1024

int main() {
    // 1. 创建监听 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    // 2. 设置端口复用（防止 "Address already in use"）
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));

    // 4. 开始监听
    listen(server_fd, 10);
    printf("服务器启动，监听端口 %d\n", PORT);

    // 5. 创建 epoll 实例
    int epfd = epoll_create(1);

    // 6. 把监听 socket 加入 epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;          // 水平触发（默认）
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    // 7. 就绪事件数组
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                // 有新连接
                int client_fd = accept(server_fd, NULL, NULL);
                printf("新客户端连接: %d\n", client_fd);

                // 新连接的 fd 也加入 epoll
                ev.events = EPOLLIN | EPOLLET;  // 用边沿触发
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

            } else {
                // 有客户端发数据
                char buf[4096];
                int nread = read(fd, buf, sizeof(buf));

                if (nread <= 0) {
                    // 客户端断开连接或出错
                    printf("客户端 %d 断开\n", fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    buf[nread] = '\0';
                    printf("收到 [%d]: %s", fd, buf);
                    // 回显
                    write(fd, buf, nread);
                }
            }
        }
    }

    close(server_fd);
    close(epfd);
    return 0;
}
```

> **编译运行**：
> ```bash
> gcc -o server server.c
> ./server
> # 另一个终端：
> nc localhost 8888
> ```

---

## 总结

| 关键词 | 说明 |
|--------|------|
| **I/O 多路复用** | 一个进程同时监控多个 fd，避免阻塞在单个 fd 上 |
| **select** | 最简单，但上限 1024，每次全量拷贝+遍历 |
| **poll** | 无上限，但仍全量拷贝+遍历，改进了输入输出分离 |
| **epoll** | Linux 专属，事件驱动，O(1) 复杂度，支持 ET/LT 模式 |
| **LT (水平触发)** | 只要有数据就通知，简单但可能重复 |
| **ET (边沿触发)** | 只在状态变化时通知一次，高效但必须一次读完 |
| **mmap** | 共享内存减少内核态/用户态数据拷贝 |
| **回调机制** | epoll 为每个 fd 注册回调，就绪时自动加入链表 |

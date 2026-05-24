# 内存管理基础

C++程序看到的不是物理内存，而是虚拟地址空间。

## 虚拟内存与进程地址空间

- 虚拟地址 ≠ 物理地址：每个进程拥有独立的虚拟地址空间（64位系统理论上是$2^{64}$，实际Linux x86_64通常只使用48位，即256TB）。CPU通过 MMU + 页表 将虚拟地址翻译为物理地址。

> 💡 **一看就懂**：同一个程序跑两遍，代码里的指针值（虚拟地址）可能一样，但背后的物理内存完全不同——每个进程有自己独立的地址空间副本。
>
> ```c
> #include <stdio.h>
> #include <stdlib.h>
> 
> int global_var = 42;          // 数据段 .data
> 
> int main() {
>     int stack_var = 10;       // 栈
>     int *heap_var = malloc(4);// 堆
>     
>     printf("代码段 main:  %p\n", main);
>     printf("数据段 glob:  %p\n", &global_var);
>     printf("栈  stack:    %p\n", &stack_var);
>     printf("堆  heap:     %p\n", heap_var);
>     free(heap_var);
>     return 0;
> }
> // 运行结果（每次地址可能不同）:
> // 代码段 main:  0x555555555149    ← 低地址，只读
> // 数据段 glob:  0x555555558010    ← 稍高
> // 栈  stack:    0x7fffffffde4c    ← 高地址
> // 堆  heap:     0x5555555592a0    ← 代码段与栈之间
> ```
- 完整的进程内存布局（远比三段式复杂）：

    ```bash
    高地址 
        ┌─────────────────────┐
        │   Kernel Space      │ ← 内核态，用户不可直接访问
    ────┼─────────────────────┤ ← TASK_SIZE (如 0x7FFFFFFFFFFF)
        │   Stack             │ ← 向下增长，有大小限制(ulimit -s)
        ├─────────────────────┤
        │   [Guard Page]      │ ← 防止栈溢出覆盖下方内存的关键机制
        ├─────────────────────┤
        │   mmap / VDSO       │ ← 动态库、mmap映射、vsyscall
        ├─────────────────────┤
        │   Heap              │ ← brk/sbrk 扩展的传统堆
        ├─────────────────────┤
        │   BSS               │ ← 未初始化全局/静态变量 (零填充)
        ├─────────────────────┤
        │   Data              │ ← 已初始化全局/静态变量
        ├─────────────────────┤
        │   Text (Code)       │ ← 只读可执行段 (.text, .rodata) 
        └─────────────────────┘
    低地址
    ```

- 关键认知：new/malloc 分配的“堆”实际上可能来自两个不同的系统调用：
  - 小内存 (<128KB): 通过 brk() 移动堆顶指针，在Data段上方扩展。
  - 大内存 (≥128KB): 通过 mmap(MAP_ANONYMOUS) 在mmap区域创建匿名映射。这两者的释放行为、碎片特性完全不同！

## 分页机制与TLB

内存是按页管理

- 页大小：通常4KB（可通过 getconf PAGESIZE 确认）。malloc(10) 实际至少占用一个页的物理内存（虽然分配器会复用）。
- 缺页中断：mmap 或 brk 返回后，物理内存并没有立即分配！只有当你第一次读写该虚拟地址时，CPU触发 Page Fault，OS才真正分配物理页并建立页表映射。这就是延迟分配。
- TLB：页表查找很慢，CPU用TLB缓存最近的页表项。频繁随机访问大块内存会导致TLB Miss，性能暴跌。这解释了为什么遍历二维数组按行比按列快得多。

## 栈的真实工作机制

栈不仅仅是“存局部变量的地方”，它是函数调用约定的核心载体。

- 栈帧结构：每次函数调用压入一个栈帧，包含返回地址、保存的寄存器、参数传递区、局部变量区。
- 栈对齐要求：x86_64 System V ABI要求栈指针RSP在函数调用前必须**16字节对齐**。不对齐会导致SSE/AVX指令崩溃或性能下降。这就是为什么编译器经常在函数开头插入`and rsp, -16`。
- Red Zone：x86_64 Linux下，叶子函数(不调用任何其他函数的函数)可以使用RSP以下128字节的“红区”而不必调整栈指针。信号处理函数会破坏红区，这是异步信号安全的隐患。
- 栈溢出检测：依赖Guard Page。如果递归过深跳过Guard Page直接访问到mmap区域，不会产生SIGSEGV而是静默数据损坏——这是最危险的bug之一。

## 内存对齐与对象布局

- 自然对齐：类型为T的变量地址必须是 alignof(T) 的倍数。double 通常8字节对齐，int 4字节对齐。
- 结构体填充：编译器在成员之间和末尾插入padding以满足对齐。

## C运行时分配器的内部机制

`malloc/free`不是系统调用，它们是用户态的内存管理器。如`brk`，`mmap`这些是操作系统给的系统调用接口，malloc是C封装的在用户态。

```bash
堆结构（如面团从低向高延伸）
低地址 ◄──────────────────────────────────────► 高地址
┌──────────┬──────────┬──────────┬─────────────────────┐
│ 已分配块A │ 空闲块B  │ 已分配块C │Top Chunk 剩余面团    │
└──────────┴──────────┴──────────┴─────────────────────┘
                                 ▲                     ▲
                            普通空闲块            永远在最右端
                           (双向链表管理)         (未被切分的原始空间)

==============================================

你的代码: malloc(64)
    │
    ▼
┌─────────────────────────────────┐
│  用户态: libc 分配器 (ptmalloc2) │
│                                 │
│  1. 检查空闲链表(fastbin/smallbin)│ ← 如果有合适的空闲块，直接返回！
│     ✅ 命中 → O(1) 返回指针      │   【99%的情况走这里，无系统调用】
│     ❌ 未命中 ↓                  │
│                                 │
│  2. 检查 Top Chunk 剩余空间      │ ← 如果堆顶还有足够空间，切割返回
│     ✅ 够大 → 切割并返回         │   【仍然无系统调用】
│     ❌ 不够 ↓                   │
│                                 │
│  3. 🔴 触发系统调用              │ ← 只有真正"没地了"才找OS
│     • 小请求 → brk() 扩展堆顶    │
│     • 大请求 → mmap() 映射新区域  │
│     获得新内存后，再切出64字节返回 │
└─────────────────────────────────┘
```

### Bins

其中`Bins`相当于空闲链表分类系统，按大小分类的空闲块回收站。glibc不是把所有空闲块放在一个链表，而是分类放，快查找

### Fast Bin

程序中最频繁的分配/释放往往是相同大小的小对象（比如64字节的节点、32字节的字符串）。如果用通用链表，每次都要遍历+合并，太慢了。

准入条件<=80B的被free的块，按 free 的时间顺序堆放。

### Top Chunk

从brk()系统调用诞生。程序第一次调用 malloc，或者现有空闲块+当前 Top Chunk 都不够用时，glibc 会向 OS 请求扩展堆。

```bash
初始状态（第一次 malloc 触发 brk）:
┌─────────────────────────────────────┐
│         整个 brk 扩展出来的空间       │ ← 全部就是 Top Chunk
└─────────────────────────────────────┘
▲                                     ▲
brk_start                           brk_end

malloc(100) 之后:
┌──────────┬──────────────────────────┐
│ 已分配100B│     Top Chunk (剩余部分)   │ ← Top Chunk 缩小了
└──────────┴──────────────────────────┘
           ▲                          ▲
      新的用户块                    brk_end (没变)
```

> 当 free() 一个紧邻 Top Chunk 左侧的块时，glibc 不会把这块放回 bin，而是直接把它吸收到 Top Chunk 中，让 Top Chunk 向左扩大。这看起来像"合并"，但本质是 Top Chunk 的边界回退，而不是两个普通空闲块的合并。

```bash
free(已分配块C) 之前:
┌──────┬──────┬──────────┬─────────────┐
│  A   │  B   │    C     │ Top Chunk   │
└──────┴──────┴──────────┴─────────────┘

free(C) 之后（C 被 Top Chunk 吸收）:
┌──────┬──────┬────────────────────────┐
│  A   │  B   │    Top Chunk (变大了!)   │ ← C 消失了，成为 Top Chunk 的一部分
└──────┴──────┴────────────────────────┘
```

#### Top Chunk生命周期

```bash
brk(+4096)
    │
    ▼
诞生: 4096B 全新 Top Chunk
    │
    ├── malloc(100) → 从左侧切100B，Top Chunk剩3996B
    ├── malloc(200) → 再切200B，Top Chunk剩3796B
    ├── free(紧邻Top的块) → Top Chunk向左吸收，变大
    ├── malloc(5000) → Top Chunk不够！
    │       │
    │       ▼
    │   brk(+8192) → Top Chunk 向右扩展 8192B
    │       │
    │       ▼
    │   现在 Top Chunk = 3796 + 8192 = 11988B
    │   继续切5000B，剩余6988B
    │
    └── ... 循环直到进程结束
```

# C风格内存管理

C 风格内存管理指通过 C 标准库函数手动控制内存——申请、使用、释放。没有 C++ 的 RAII 和智能指针，全靠程序员自觉管理，因此也是理解操作系统内存管理原理的好入口。

## 一、堆内存分配函数

### 1.1 `malloc` —— 分配内存块

```c
#include <stdlib.h>
void *malloc(size_t size);
```

- 分配 `size` 字节，返回指向这块内存的指针
- 失败返回 `NULL`（**必须检查返回值！**）
- 返回地址至少 16 字节对齐（x86_64 上满足所有类型的对齐要求）
- 分配的内存**不保证清零**，内容为上次使用留下的脏数据

> 💡 **一看就懂**：
>
> ```c
> int *p = (int*)malloc(5 * sizeof(int));  // 申请 5 个 int 的空间
> if (!p) { /* 处理失败 */ }
> p[0] = 10;  p[1] = 20;                   // 像数组一样用
> free(p);                                  // 用完释放
> ```
>
> malloc 返回的是一块**未初始化**的内存，里面的值是"脏"的（上次使用留下的数据），使用前必须自己赋值或 memset 清零。

```bash
malloc(64) 在 glibc 内部的查找路径:

  p = malloc(64)
      │
      ├── ① fastbin 有空闲?      ──❌ 没有
      ├── ② 其他 bin 有空闲?     ──❌ 没有
      ├── ③ Top Chunk 空间够?    ──✅ 够 → 切 64B 返回
      │                             整个过程 0 次系统调用
      └── ④ Top Chunk 也不够?    ──→ brk() 向内核申请新内存 → 再切
                                     这次触发了系统调用
```

> **关键认识**：大部分 `malloc` 不触发系统调用，glibc 优先从已申请的空闲内存中复用。只有全部用完了才找内核要。

### 1.2 `calloc` —— 清零分配

```c
#include <stdlib.h>
void *calloc(size_t nmemb, size_t size);
```

- 分配 `nmemb * size` 字节并**全部置为 0**
- 相当于 `malloc(nmemb * size) + memset(..., 0)`，但底层可能更高效

> **为什么叫清零分配**：`calloc` 分配的内存保证每个字节都是 0，适合需要初始化为空的场景（如数组、哈希表）。但注意"清零"在内核层面有优化，不一定真的逐个字节写一遍。

> 💡 **malloc vs calloc 对比**：
>
> ```c
> int *a = malloc(5 * sizeof(int));     // 内容未知（脏数据）
> int *b = calloc(5, sizeof(int));      // 内容全是 0
>
> printf("%d\n", a[0]);  // 随机值（可能是上次遗留的数据）
> printf("%d\n", b[0]);  // 一定是 0
>
> free(a); free(b);
> ```
>
> 经验：如果你需要初始化为 0 就用 `calloc`，否则用 `malloc` 更快。

### 1.3 `realloc` —— 调整内存块大小

```c
#include <stdlib.h>
void *realloc(void *ptr, size_t new_size);
```

尝试**原地扩展**已有内存块；如果后面空间不够，就另找一块新内存、拷贝旧数据过去、释放旧块。

```bash
realloc(ptr, 256) 的三种情况:

情况 A: 后面是空闲 → 直接扩大        ✅ 无拷贝
情况 B: 后面已分配 → 另找新家拷贝     ⚠️ 有拷贝开销
情况 C: 缩小 → 只改内部记录          ✅ 无操作
```

#### ⚠️ 常见错误

```c
// ❌ 错误：realloc 失败返回 NULL，原指针丢了！
void *p = malloc(100);
p = realloc(p, 1000000);     // 失败 → p = NULL，旧内存泄漏！

// ✅ 正确做法
void *tmp = realloc(p, 1000000);
if (tmp) p = tmp;            // 成功才更新指针
else { /* 错误处理，p 仍然有效 */ }
```

### 1.4 `free` —— 释放内存

```c
#include <stdlib.h>
void free(void *ptr);
```

- 释放 `malloc / calloc / realloc` 返回的指针
- 传 `NULL` 是安全的（什么都不做）

#### 释放后内存去了哪里？

```bash
free(p)
    │
    ├── 小块(≤80B) → 放入 fastbin（LIFO 栈，不合并，下次快速复用）
    ├── 中块 → 合并相邻空闲块 → 放入合适 bin
    ├── 紧邻堆顶 → 归还给 Top Chunk（可能触发 brk 收缩）
    └── 大块(mmap 分配) → munmap 立即归还内核
```

> **重要**：`free` 通常**不立即把内存归还给操作系统**。glibc 把这块内存标记为空闲，下次 `malloc` 可以直接复用。这就是为什么 free 一块大内存后，任务管理器里进程的 RSS 可能没降——内存还在进程手里，只是标记为"可重用"。

> 💡 **验证 free 不立即归还**：
>
> ```c
> #include <stdio.h>
> #include <stdlib.h>
> #include <unistd.h>
> 
> int main() {
>     char *p = malloc(100 * 1024 * 1024);  // 申请 100MB
>     printf("申请后 按回车 → RSS 应该涨了 100MB\n");
>     getchar();
>     
>     free(p);
>     printf("释放后 按回车 → RSS 可能没降（内存还在进程手里）\n");
>     getchar();
>     // 按 Ctrl+C 退出，进程结束内存才真正归还
>     return 0;
> }
> ```
> 运行后另开终端 `htop` 或 `top` 观察 RES 列，会发现 free 后 RSS 不降。

---

## 二、栈内存分配

### 2.1 `alloca` —— 栈上动态分配

```c
#include <alloca.h>
void *alloca(size_t size);
```

- 在**当前函数栈帧**上分配 `size` 字节
- **不用 free**，函数返回时自动回收（栈指针恢复）
- 速度极快：一条汇编指令的事

```bash
栈上的 Guard Page 机制:

 高地址
  ┌──────────────────┐
  │  已用栈空间        │
  ├──────────────────┤
  │  alloca 分配区     │ ← RSP 往下移
  ├──────────────────┤ ← RSP (新)
  │  [Guard Page]     │ ← 不可读写，越界触发段错误
  ├──────────────────┤
  │  Heap / mmap 区域  │
  └──────────────────┘
 低地址
```

#### ⚠️ 使用注意

```c
// ✅ 适合：小尺寸临时缓冲区
void process(const char *path) {
    char *copy = alloca(strlen(path) + 1);
    strcpy(copy, path);
    // 不用 free，函数返回自动回收
}

// ❌ 绝不要在循环里用 alloca！
for (int i = 0; i < 100000; i++) {
    char *buf = alloca(4096);  // 栈不会释放直到函数返回 → 栈溢出崩溃
}
// 为什么？alloca 只在函数返回时统一回收栈指针（特殊时机），
// 不像 free 可以逐块释放。
// 循环 10 万次相当于一次性在栈上累积 4096*10万 ≈ 400MB，
// 栈总大小通常只有 8MB（ulimit -s），远超限制直接段错误。
```

---

## 三、常用内存操作函数

### 3.1 `memset` —— 设置内存

```c
#include <string.h>
void *memset(void *s, int c, size_t n);
```

把从 `s` 开始的 `n` 个字节都设为 `c`。最常用的是 `memset(p, 0, size)` 清零。

### 3.2 `memcpy` / `memmove` —— 复制内存

```c
#include <string.h>
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
```

- `memcpy`：假设 dest 和 src 不重叠，**更快**
- `memmove`：处理重叠情况，**保证正确**

```c
// 重叠场景
char buf[] = "Hello World!";
memcpy(buf + 2, buf, 6);     // ❌ 重叠，未定义行为！
memmove(buf + 2, buf, 6);    // ✅ 正确: "HeHello!"
```

### 3.3 `memcmp` —— 内存比较

```c
#include <string.h>
int memcmp(const void *s1, const void *s2, size_t n);
```

按字节比较两个内存区域，返回第一个不同字节的差值。与 `strcmp` 不同：`memcmp` 遇到 `\0` **不会**停止。

---

## 四、系统调用层

`malloc/free` 不是系统调用，它们是**用户态的内存管理器**。当用户态内存不够时，才会通过系统调用向内核申请。

### 4.1 `brk` / `sbrk` —— 堆顶伸缩

```c
#include <unistd.h>
int brk(void *addr);
void *sbrk(intptr_t increment);
```

- `brk`：把堆的结束地址设为 `addr`
- `sbrk(0)`：返回当前堆顶位置（不修改）
- `sbrk(n)`：把堆扩大 `n` 字节，返回原来的堆顶地址

```bash
进程内存布局中的堆:
低地址
  ┌──────────────┐
  │  .text (代码) │
  ├──────────────┤
  │  .rodata     │
  ├──────────────┤
  │  .data/.bss  │
  ├──────────────┤ ← 堆起点 (start_brk)
  │              │
  │    Heap      │ ← ptmalloc 管理的小块内存
  │              │
  ├──────────────┤ ← 堆顶 (brk)    ← sbrk/brk 移动这里
  │  未映射区域   │
  └──────────────┘
高地址

sbrk(+4096) → 堆向上扩展 4KB
sbrk(-4096) → 堆向下收缩 4KB，归还物理内存
```

### 4.2 `mmap` —— 大块内存映射

对于大块分配（默认 >128KB），glibc 不走 `brk`，而是用 `mmap`：

```c
// glibc 内部近似实现
void *p = mmap(NULL, size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
```

- `mmap` 分配的块是**页对齐**的（4KB 整数倍）
- 用 `munmap` 释放时**立即归还操作系统**
- 不同 `mmap` 块之间不连续，各有独立映射

---

## 五、C 内存管理的经典问题

### 5.1 内存泄漏

```c
void leak() {
    char *p = malloc(1024);
    // 忘记 free(p)
}
// p 变成孤儿，函数返回后找不到它，1024 字节泄露
```

> 进程退出时内核会回收所有内存，但长期运行的程序（服务器）中泄漏会逐渐吞噬内存，最终导致系统 OOM（Out of Memory）。

> 💡 **如何发现泄漏**：
> ```bash
> # 编译时加 AddressSanitizer，运行时会报告泄漏
> gcc -fsanitize=address -g leak.c -o leak
> ./leak
> # 输出: Leaked 1024 bytes at 0x...
> ```
>
> 或在终端用 Valgrind：
> ```bash
> valgrind ./leak
> # 输出: definitely lost: 1,024 bytes
> ```

### 5.2 悬空指针 / Use-After-Free

```c
char *p = malloc(100);
free(p);          // 释放了
strcpy(p, "hi");  // ❌ p 已是悬空指针！写入了已释放的内存
```

> 可怕之处：free 后内存可能还没还给系统，写入"好像能工作"。但这块内存可能已被下一次 `malloc` 分配出去——你在同时修改两个对象的数据。

### 5.3 缓冲区溢出

```c
char *buf = malloc(16);
buf[16] = 0xFF;   // ❌ 越界写入！覆盖了下一个 chunk 的头部元数据
free(buf);        // glibc 校验发现元数据被篡改 → 崩溃
```

> 这是经典漏洞——**堆溢出攻击**。攻击者通过溢出覆盖相邻 chunk 的元数据，构造 fake chunk，最终实现任意地址读写。

### 5.4 Double Free

```c
free(p);
free(p);  // ❌ 双重释放
```

> glibc 检测规则是"不能**连续** free 同一个块两次"，但中间插一个别的就绕过了：
>
> ```c
> free(p);              // p 进 tcache 链表
> free(q);              // q 进 tcache（绕过检查，因为上一次 free 的是 p 不是 p）
> free(p);              // p 又进 tcache ← 现在链表: [p] → [q] → [p]（p 出现两次）
> ```
>
> tcache 本质是单向链表，取数据按"后进先出"。接着三次 malloc：
>
> ```c
> malloc(64);  // 取出 p  ← 返回的地址就是 p
> malloc(64);  // 取出 q  ← 返回的地址就是 q
> malloc(64);  // 取出 p  ← 又返回 p！但上一轮拿 p 的人还在用这块内存
> ```
>
> 现在**两个不同的地方都持有 p 这块内存**。攻击者可以通过其中一个写数据，另一个读——实现越权访问。如果再进一步把链表里的 next 指针改成任意地址，下次 malloc 就会返回那个地址，实现**任意地址写**（tcache poisoning）。

---

## 六、最佳实践

```c
// 1. malloc 后检查 NULL
int *p = malloc(n * sizeof(int));
if (!p) { /* 处理错误 */ }

// 2. free 后指针置 NULL（防止悬空指针误用）
free(p);
p = NULL;

// 3. 成对出现：malloc/free、calloc/free、mmap/munmap

// 4. 谁分配谁释放 —— 避免函数内分配、外部释放的割裂风格

// 5. 小对象栈上分配优先（局部数组、alloca），大对象再用堆
```

# C++风格内存管理

C++ 内存管理相比 C 最大的区别是：**从"手动管理"变为"依托 RAII 自动管理"**。RAII（Resource Acquisition Is Initialization）的核心思想是——资源在构造时获取，在析构时释放，编译器自动调用构造函数和析构函数，杜绝忘记释放。

## 一、new / delete —— C++ 的 malloc/free

### 1.1 对比一览

| 操作 | C 风格 | C++ 风格 |
|------|--------|---------|
| 分配内存 | `malloc(size)` | `new T` |
| 释放内存 | `free(p)` | `delete p` |
| 分配数组 | `malloc(n * sizeof(T))` | `new T[n]` |
| 释放数组 | `free(p)` | `delete[] p` |
| 分配+构造 | 没有 | `new T(args)` **构造对象** |
| 析构+释放 | 没有 | `delete p` **先析构再释放** |

### 1.2 `new` 比 `malloc` 多了什么？

```bash
// C: malloc 只管"给块内存"
int *p = (int*)malloc(sizeof(int));
*p = 42;                    // 自己赋值初始化

// C++: new 做了两件事
int *p = new int(42);       // ① 分配内存 ② 调用构造函数初始化

// 对象的差别更明显
// C 风格：
struct Foo *p = (Foo*)malloc(sizeof(Foo));
Foo_init(p);                // 手动调用"构造函数"

// C++ 风格：
Foo *p = new Foo(args);     // 分配 + 构造，一步到位
delete p;                   // 析构 + 释放，一步到位
```

> 💡 **核心区别**：`malloc/free` 只操作内存（分配/释放字节），`new/delete` 操作**对象的生命周期**（构造/析构）。

### 1.3 `new[]` / `delete[]` —— 数组

```c
// C 风格
int *arr = (int*)malloc(10 * sizeof(int));
free(arr);

// C++ 风格
int *arr = new int[10];     // 分配 10 个 int
delete[] arr;               // ❗ 必须用 delete[]，不是 delete
```

> ⚠️ **重要**：`new[]` 必须配 `delete[]`，`new` 配 `delete`。混用会导致未定义行为（可能崩溃或内存泄漏）。因为 `new[]` 会在内存头部记录数组长度，`delete[]` 需要读取这个长度来逐个析构。

```bash
new int[3] 的内存布局:
┌─────────┬─────────┬─────────┬─────────┐
│ [计数n] │ arr[0]  │ arr[1]  │ arr[2]  │
│ 4或8B   │         │         │         │
└─────────┴─────────┴─────────┴─────────┘
▲         ▲
│         └── new 返回的地址（程序员看到的）
└──────────── 隐藏的数组长度（delete[] 需要用到）
```

### 1.4 `new` 失败怎么办？

```c
// C 风格：malloc 返回 NULL → 检查
int *p = (int*)malloc(100);
if (!p) { /* 处理 */ }

// C++ 风格：new 失败**抛异常**，不返回 NULL
try {
    int *p = new int[100000000000];  // 可能 bad_alloc
} catch (std::bad_alloc &e) {
    // 处理分配失败
}

// 如果非要 C 风格的 NULL 返回，用 nothrow
int *p = new (std::nothrow) int[100];
if (!p) { /* 处理 */ }
```

---

## 二、构造与析构 —— 内存视角的全过程

构造函数和析构函数不只是"初始化数据"和"清理数据"——它们在内存和操作系统层面有一系列严谨的步骤。

### 2.1 栈上对象：`Foo f;` 发生了什么？

```cpp
class Foo {
    int a;
    double b;
    std::string s;
public:
    Foo() : a(0), b(0.0), s("hello") {}
    ~Foo() {}
};

void test() {
    Foo f;          // 构造函数调用
    // ...
}                   // 析构函数调用
```

#### 构造过程（内存视角）

```bash
步骤 1: 分配栈空间（编译器在函数序言中完成）
  ┌──────────────────┐  ← RBP（栈基址）
  │  返回地址         │
  ├──────────────────┤
  │  保存的 RBP       │
  ├──────────────────┤  ← RSP 下移 sizeof(Foo) 字节
  │  Foo f 的空间     │    ⚠️ 此时内存上是脏数据，对象尚未"诞生"
  │  [a: 随机值]      │
  │  [b: 随机值]      │  
  │  [s: 随机值]      │
  └──────────────────┘
        │
        ▼
步骤 2: 编译器插入构造调用（按成员声明顺序）
  ┌──────────────────┐
  │ ① a = 0          │  ← 直接赋值（基础类型）
  ├──────────────────┤
  │ ② b = 0.0        │  ← 直接赋值
  ├──────────────────┤
  │ ③ s("hello")     │  ← 调用 std::string 的构造函数
  │    │              │     内部可能 malloc 分配堆内存存 "hello"
  │    ▼              │
  │   s.data → 堆区   │  ← string 的指针指向堆上分配的 "hello\0"
  └──────────────────┘
        │
        ▼
步骤 3: 如果 Foo 有虚函数 —— 设置 vptr
  ┌──────────────────┐
  │ vptr → Foo_vtable│  ← 指向 .rodata 中的虚函数表
  ├──────────────────┤
  │ a = 0            │
  ├──────────────────┤
  │ b = 0.0          │
  ├──────────────────┤
  │ s: [data│len│cap]│
  └──────────────────┘
  对象正式"诞生"
```

#### 析构过程（内存视角）

```bash
步骤 1: 调用 ~Foo()
  ┌──────────────────┐
  │ ① 先执行用户写的析构体 │
  ├──────────────────┤
  │ ② 按成员声明逆序调用   │  ← 先构造的后析构
  │    成员的析构函数      │
  │    s.~string() →      │
  │    free(s.data)       │  ← string 释放堆内存
  ├──────────────────┤
  │ ③ 如果有虚函数 →     │
  │    vptr 改回基类版本  │  ← 防止析构中调用虚函数访问已销毁的子类
  └──────────────────┘
        │
        ▼
步骤 2: 函数返回，恢复 RSP
  ┌──────────────────┐  ← RSP 恢复（栈帧回退）
  │  内存还在，但对象   │
  │  已经"死亡"        │  ← 这块栈内存下次被其他函数覆盖
  └──────────────────┘
  对象正式"死亡"
```

#### 验证一下

```cpp
#include <cstdio>

struct Step {
    Step(const char *n) : name(n) { printf("构造 %s\n", name); }
    ~Step()                       { printf("析构 %s\n", name); }
    const char *name;
};

struct Inner {
    Step a{"a"}, b{"b"}, c{"c"};
    Inner() { printf("-- Inner 构造体 --\n"); }
    ~Inner() { printf("-- Inner 析构体 --\n"); }
};

int main() { Inner x; }
// 输出:
// 构造 a      ← 成员按声明顺序构造
// 构造 b
// 构造 c
// -- Inner 构造体 --  ← 最后执行构造函数体
// -- Inner 析构体 --  ← 先执行析构函数体
// 析构 c              ← 成员按声明逆序析构
// 析构 b
// 析构 a
```

### 2.2 堆上对象：`new Foo` 比栈对象多了什么？

栈对象：**编译时**就知道要分配多少空间，函数序言中一条 `sub rsp, N` 搞定。

堆对象：**运行时**才去堆里找空闲内存，涉及更多步骤。

```bash
Foo *p = new Foo(10);

// ===== new 表达式 =====
// 编译器将其拆为两步（无法合为一个"原子操作"）：
// ① operator new(sizeof(Foo))   → 分配内存（本质是 malloc）
// ② Foo::Foo(10)               → 在分配的内存上构造
          │
          ▼
┌──────────────────────────────────────────────┐
│ 步骤 1: operator new → 内部调用 malloc       │
│                                              │
│   ┌────────────────────────────────┐         │
│   │ malloc(sizeof(Foo))            │         │
│   │   ├── fastbin 有空闲? → O(1) 返回         │
│   │   ├── Top Chunk 够? → 切割返回            │
│   │   └── 都不够? → brk() 系统调用            │
│   └────────────────────────────────┘         │
│         ↓                                     │
│         返回堆地址如 0x5555555592a0             │
│                                              │
│ 步骤 2: 在这块内存上调用构造函数                │
│                                              │
│   地址 0x5555555592a0 处的内存：               │
│   ┌────────────────────────────┐              │
│   │ 构造前: 脏数据              │              │
│   │ 构造后:                     │              │
│   │   vptr → Foo_vtable         │              │
│   │   a = 10                    │              │
│   └────────────────────────────┘              │
└──────────────────────────────────────────────┘

// ===== delete 表达式 =====
delete p;

// 编译器同样拆为两步：
// ① p->~Foo()               → 析构对象
// ② operator delete(p)      → 释放内存（本质是 free）
```

#### OS 视角：new 大对象 vs 小对象

```bash
new Foo;          // 小对象（假设 32 字节）
    │
    ▼
通常走 malloc 内部缓存（fastbin / Top Chunk）
→ 0 次系统调用 ✅  （内存已提前从内核申请好了）

new BigObject[1000000];  // 大对象（约 8MB）
    │
    ▼
malloc 发现 >= 128KB → 用 mmap 匿名映射
→ 1 次 mmap 系统调用 ← 内核在进程地址空间映射新区域
→ 1 次缺页中断      ← 首次写入时内核分配物理页
```

### 2.3 继承链中的构造与析构

当有继承关系时，构造/析构的顺序至关重要：

```cpp
class Base {
public:
    Base()  { printf("Base构造\n"); }
    ~Base() { printf("Base析构\n"); }
};

class Derived : public Base {
    int *data;
public:
    Derived() : Base() { 
        data = new int[100];    // ① 先构造 Base
        printf("Derived构造\n"); // ② 再构造 Derived
    }
    ~Derived() {
        delete[] data;          // ① 先析构 Derived
        printf("Derived析构\n"); 
    }                           // ② 再析构 Base（编译器自动插入）
};
```

#### 内存布局演变动画

```bash
// ===== 构造 Derived 对象 =====
栈/堆上分配 Derived 大小的空间
┌────────────────────┐
│  脏数据             │  ← 分配完，尚未构造
└────────────────────┘

调用 Derived 构造函数
  │
  ├── ① 先调 Base 构造函数
  │     ┌────────────────────┐
  │     │ Base 部分:         │
  │     │   vptr → Base_vtbl │  ← 此时 vptr 指向 Base
  │     │   base_data = 0    │
  │     └────────────────────┘
  │
  └── ② 再执行 Derived 构造体
        ┌────────────────────┐
        │ Base 部分:         │
        │   vptr → Derived_vtbl│  ← vptr 改为 Derived 版本
        │   base_data = 0    │
        ├────────────────────┤
        │ Derived 部分:      │
        │   data = 0x...     │  ← 堆上新分配的 100 个 int
        └────────────────────┘
  对象正式可用

// ===== 析构 Derived 对象 =====
调用 ~Derived()
  │
  ├── ① 执行 ~Derived() 体
  │      delete[] data;     ← 先释放 Derived 申请的堆内存
  │
  └── ② 编译器自动插入 ~Base()
         vptr → Base_vtbl   ← 防止析构中调用虚函数访问已销毁的部分
```

> **关键原则**：
> - 构造：**先基类 → 后子类**（像盖楼，先打地基）
> - 析构：**先子类 → 后基类**（像拆楼，先拆上层）
> - 子类构造时，vptr 会从"指向基类"变为"指向子类"（vptr 更新发生在构造体执行前）

### 2.4 `new[]` 构造数组时发生了什么？

```cpp
Foo *arr = new Foo[3];  // 构造 3 个 Foo 对象
delete[] arr;           // 析构 3 个 Foo 对象
```

```bash
new Foo[3] 的内存布局:

隐藏的计数             三个对象连续排列
┌──────┬────────┬────────┬────────┐
│  [3] │  Foo[0]│  Foo[1]│  Foo[2]│
│ 8字节 │ 构造①  │ 构造②  │ 构造③  │
└──────┴────────┴────────┴────────┘
▲      ▲
│      └── new 返回的地址（arr）
│
└── 编译器偷偷放的数组长度

delete[] arr 时:
1. 读取 arr[-8] 得到长度 3
2. 从后往前逐个析构: Foo[2] → Foo[1] → Foo[0]（逆序）
3. 释放整块内存
```

> ⚠️ 如果你用 `delete arr`（没加 `[]`）释放 `new[]` 分配的内存：
> - 编译器不会读取隐藏的计数，只调用第 0 个元素的析构
> - 第 1 个和第 2 个对象泄漏（如果有堆成员）
> - 然后释放整块内存 → 未定义行为（可能崩溃）

---

## 三、RAII —— C++ 内存管理的灵魂

RAII 是 C++ 最核心的 idiom。它的本质是：**用对象的生命周期管理资源**。

### 2.1 原理

```bash
{
    Foo f;            // 构造函数：获取资源（如 new 了一块内存）
    // ... 使用 f ...
}                     // } 出作用域 → 析构函数自动调用：释放资源
```

### 2.2 一个对比看懂 RAII 的价值

```c
// ========== C 风格 ==========
void process() {
    int *p = (int*)malloc(100 * sizeof(int));
    if (!p) return;
    
    // ... 中间有 10 行代码 ...
    if (something_wrong) {
        free(p);        // ❗ 每个提前返回处都要记得 free
        return;
    }
    // ... 又有 10 行代码 ...
    
    free(p);            // ❗ 函数末尾也要 free
}
// 问题：漏一个 free 就泄漏了，复杂函数极难保证

// ========== C++ RAII 风格 ==========
void process() {
    std::vector<int> v(100);  // 构造时自动分配内存
    
    // ... 中间有 10 行代码 ...
    if (something_wrong) {
        return;               // ✅ 不用管！v 析构时自动释放
    }
    // ... 又有 10 行代码 ...
    
}                             // ✅ 出作用域，v 自动析构释放
```

> **本质差异**：C 的 `free` 是**人记**，C++ 的析构是**编译器记**。人记就会漏，编译器不会。

### 2.3 RAII 不止管内存

RAII 可以管理任何需要配对获取/释放的资源：

```cpp
// 文件
std::ofstream file("test.txt");   // 构造时打开
// ... 写文件 ...
}                                 // 析构时自动 close

// 锁
{
    std::lock_guard<std::mutex> lock(mtx);  // 构造时加锁
    // ... 临界区 ...
}                                           // 析构时自动解锁

// 自定义 RAII
class FileGuard {
    FILE *f;
public:
    FileGuard(const char *name) { f = fopen(name, "r"); }
    ~FileGuard() { if (f) fclose(f); }
    // ... 读写方法 ...
};
```

---

## 四、智能指针 —— 详细用法与原理

智能指针是 RAII 在堆内存上的具体应用。本质上是一个**包装了原始指针的类**，在析构函数里自动调用 `delete`，杜绝忘记释放。

### 关于 `<memory>` 库

智能指针来自 C++ 标准库头文件 `<memory>`。这个头文件提供了现代 C++ 内存管理的核心工具，主要包括：

| 组件 | 说明 |
|------|------|
| `std::unique_ptr` | 独占所有权的智能指针 |
| `std::shared_ptr` | 共享所有权的智能指针（引用计数） |
| `std::weak_ptr` | 弱引用，配合 `shared_ptr` 打破循环 |
| `std::make_unique` | 创建 `unique_ptr` 的工厂函数（C++14） |
| `std::make_shared` | 创建 `shared_ptr` 的工厂函数（C++11） |
| `std::allocate_shared` | 带自定义分配器的 `make_shared` |
| `std::enable_shared_from_this` | 让对象安全地获取管理自己的 `shared_ptr` |
| `std::owner_less` | 智能指针的比较函数对象 |
| `std::auto_ptr` | **已废弃**（C++17 移除），不要使用 |

> `<memory>` 不止智能指针，还包含 `allocator`（标准库分配器）、`pointer_traits` 等底层工具，但日常 C++ 开发中接触最多的就是上面列出的智能指针家族。

### 补充概念：完美转发（Perfect Forwarding）

`make_unique` 和 `make_shared` 能把参数原封不动地传给对象的构造函数，靠的就是**完美转发**。

#### 为什么需要"转发"？

```cpp
// 假设你想写一个"万能创建函数"，把参数转给构造函数
auto p = std::make_unique<Foo>(10, 20);  // 把 10 和 20 "转发"给 Foo 的构造函数
```

问题在于：参数可能是**左值**（有名字的变量，如 `int x = 10`）也可能是**右值**（临时值，如字面量 `10`、函数返回值）。直接传参有时会触发不必要的拷贝。

#### 通俗理解

```cpp
int a = 1;

auto p1 = std::make_unique<Foo>(a, 2);
// a 是左值（有名字） → 传引用，不拷贝
// 2 是右值（字面量） → 直接传，允许移动
```

**完美转发**：参数来时是左值就按左值传，是右值就按右值传——**原样传递，不额外拷贝，不丢失信息**。

### 4.1 `unique_ptr` —— 独占所有权

一个对象同一时刻只能被一个 `unique_ptr` 持有。不能拷贝，只能移动。

#### 基本用法

```cpp
#include <memory>

// 创建（推荐）
auto p1 = std::make_unique<int>(42);     // int*，值为 42

// 创建对象
struct Foo { Foo(int a, int b) {...} };
auto p2 = std::make_unique<Foo>(10, 20); // 完美转发参数

// 像普通指针一样用
*p1 = 100;
p2->method();
```

#### 不能拷贝，只能移动

```cpp
auto a = std::make_unique<int>(10);
// auto b = a;            // ❌ 编译错误：不能拷贝
auto c = std::move(a);    // ✅ 移动：a 变成空，c 持有所有权
// 此时 a 是 nullptr，用 a 会崩溃，但不会 double free

// 典型场景：函数返回 unique_ptr（自动移动）
std::unique_ptr<Foo> createFoo() {
    return std::make_unique<Foo>(1, 2);  // 自动移动，不需要 std::move
}
```

#### 自定义删除器

```cpp
// 默认 delete，但可以改成其他释放方式
auto fileDeleter = [](FILE *f) { fclose(f); };
std::unique_ptr<FILE, decltype(fileDeleter)> 
    p(fopen("test.txt", "r"), fileDeleter);
// 出作用域自动 fclose，不用手动调
```

#### 作为类成员

```cpp
class Owner {
    std::unique_ptr<Foo> m_foo;     // 成员变量
public:
    Owner() : m_foo(std::make_unique<Foo>()) {}
    // 不需要定义析构函数！m_foo 自动 delete
    // 不需要定义拷贝构造/赋值（Owner 不可拷贝，符合语义）
};
```

#### 在容器中

```cpp
std::vector<std::unique_ptr<Foo>> vec;
vec.push_back(std::make_unique<Foo>(1, 2));  // ✅
// vec.push_back(fooPtr);                     // ❌ 不能拷贝
vec.push_back(std::move(someUniquePtr));      // ✅ 移动

// 遍历
for (auto &p : vec) {   // 注意是 auto&，因为 unique_ptr 不能拷贝
    p->method();
}
```

#### 内存开销

```cpp
// unique_ptr<T> 的大小 = T* 的大小，零开销抽象
static_assert(sizeof(std::unique_ptr<int>) == sizeof(int*));
// 除非用了自定义删除器且删除器有状态
```

---

### 4.2 `shared_ptr` —— 共享所有权

多个 `shared_ptr` 可以指向同一个对象。内部维护一个**引用计数**，当最后一个 `shared_ptr` 被销毁时，才释放对象。

#### 基本用法

```cpp
auto p1 = std::make_shared<int>(42);  // 引用计数 = 1
{
    auto p2 = p1;                     // 拷贝，引用计数 = 2
    *p2 = 100;
    // p2 出作用域析构，引用计数 = 1
}
// p1 出作用域析构，引用计数 = 0 → 释放 int
```

#### 内部结构：为什么 `shared_ptr` 是 16 字节？

`shared_ptr` 存了**两个指针**，所以是 16 字节（x86_64 上每个 8 字节）：

```bash
shared_ptr<Foo> 内部:
┌──────────────┐  ┌──────────────┐
│  指针1 → Foo  │  │  指针2 → ???  │
└──────────────┘  └──────────────┘
```

**指针1** 指向 `Foo` 对象，那**指针2**指向什么？

##### 先想一个问题

多个 `shared_ptr` 共享同一个对象，怎么知道"最后一个析构了，可以释放了"？

```cpp
auto p1 = std::make_shared<Foo>();
auto p2 = p1;   // 两个 shared_ptr 指向同一个 Foo
// p2 先析构 → 此时不能释放 Foo，因为 p1 还在用
// p1 析构 → 没人用了，可以释放了
```

需要一个**所有 shared_ptr 都能看到的计数器**。

- 不能放 `Foo` 里（`Foo` 是用户写的类，没预留这个位置）
- 不能放某个 `shared_ptr` 内部（别的 shared_ptr 看不到）

所以 C++ 在堆上**额外分配一小块内存**来存这个计数器——这就是**控制块**。

```bash
堆上的实际内存布局（用 make_shared）:

┌──────────────────┬──────────────────────────┐
│                  │                          │
│   Foo 对象本身   │     控制块（记账本）       │
│  （你关心的数据） │    ┌─────────────────┐   │
│                  │    │ 计数器 = 2       │  │
│                  │    │ （当前两人在用）  │   │
│                  │    └─────────────────┘   │
└──────────────────┴──────────────────────────┘
▲                                              ▲
│                                              │
指针1 → Foo 对象                     指针2 → 控制块
（shared_ptr 第一个指针）          （shared_ptr 第二个指针）
```

##### 完整过程

```cpp
// 步骤 1: 创建
auto p1 = std::make_shared<Foo>();
// 堆内存: [ Foo对象 ][ 控制块{计数器=1} ]
//           ↑           ↑
//          p1.指针1    p1.指针2

// 步骤 2: 拷贝
auto p2 = p1;  // p2 的指针也指向同一块内存
// [ Foo对象 ][ 控制块{计数器=2} ]
//                            ↑
//                      拷贝一次 +1

// 步骤 3: p2 析构
// 计数器 -1 → [ Foo对象 ][ 控制块{计数器=1} ]
// 还没到 0，不动 Foo

// 步骤 4: p1 析构
// 计数器 -1 → 计数器 = 0 → 释放整块内存
```

> **一句话**：控制块就是 `shared_ptr` 们共同维护的"记账本"，记着还有多少人在用这个对象。`make_shared` 把"对象"和"记账本"放在一块连续内存里。如果自己 `new` 再传给 `shared_ptr`，那就是两次独立分配，不连续。

#### `make_shared` vs `new` 的性能差异

```cpp
// ✅ make_shared: 一次分配（对象 + 控制块在一起）
auto p = std::make_shared<Foo>();
// 内存: |      Foo      |  控制块(refcount...)  |  ← 连续

// ❌ 用 new: 两次分配
std::shared_ptr<Foo> p(new Foo());
// 内存: |      Foo      |   ← operator new 分配
//       |  控制块(refcount...) |   ← shared_ptr 构造时再分配一次
```

> `make_shared` 的优点：一次堆分配（更少碎片+更快）、异常安全。
> 缺点：控制块和对象在同一块内存，对象要等控制块一起释放（weak_ptr 存在时对象内存不能立即回收）。

#### 自定义删除器

```cpp
// shared_ptr 支持自定义删除器（不影响类型）
auto p = std::shared_ptr<FILE>(fopen("a.txt", "r"), fclose);
// 类型还是 shared_ptr<FILE>，删除器信息存在控制块里
```

#### 注意循环引用

```cpp
struct Node {
    std::shared_ptr<Node> next;  // ❌ 循环引用泄漏
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->next = a;    // a 和 b 互相引用，引用计数永远到不了 0 → 泄漏！
```

---

### 4.3 `weak_ptr` —— 打破循环引用

`weak_ptr` 不增加引用计数，只是**观察** `shared_ptr` 管理的对象。

#### 改正上面的循环引用

```cpp
struct Node {
    std::weak_ptr<Node> next;     // ✅ weak_ptr，不增加计数
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->next = a;    // 引用计数：a=1, b=1（weak 不计入）
                // 出作用域正常释放 ✅
```

#### `lock()` —— 安全访问

```cpp
std::weak_ptr<Foo> w = std::make_shared<Foo>();

// 使用前必须 lock()，因为对象可能已被释放
if (auto sp = w.lock()) {   // lock() 返回 shared_ptr
    sp->method();           // ✅ 对象还在
} else {
    // 对象已被释放
}
```

#### 观察者模式

```cpp
class Subject {
    std::vector<std::weak_ptr<Observer>> observers;  // 不拥有观察者
public:
    void notify() {
        for (auto &w : observers) {
            if (auto sp = w.lock()) {
                sp->update();   // ✅ 观察者还在
            }
            // 如果观察者已销毁，w.lock() 返回 nullptr，跳过
        }
    }
};
// 观察者销毁时自动从 subject 解除，不需要手动 unsubscribe
```

---

### 4.4 `enable_shared_from_this`

当类内部需要获取管理自己的 `shared_ptr` 时使用。

```cpp
struct Foo : std::enable_shared_from_this<Foo> {
    std::shared_ptr<Foo> getShared() {
        return shared_from_this();  // ✅ 返回管理自己的 shared_ptr
    }
};

auto p = std::make_shared<Foo>();
auto q = p->getShared();   // 引用计数 +1，p 和 q 共享所有权

// ⚠️ 必须在 shared_ptr 管理下调用
// Foo f;
// f.getShared();  // ❌ 崩溃：对象不在 shared_ptr 管理下
```

#### 原理

```bash
enable_shared_from_this 内部维护一个 weak_ptr<Foo>。
当 shared_ptr 构造时，如果 Foo 继承了 enable_shared_from_this，
shared_ptr 的构造器会把 this 的 weak_ptr 设置好。

shared_from_this() 等价于:
  return std::shared_ptr<Foo>(this_weak_.lock());
```

---

### 4.5 三种智能指针对比速查

| 特性 | `unique_ptr` | `shared_ptr` | `weak_ptr` |
|------|-------------|-------------|-----------|
| 所有权 | 独占 | 共享 | 观察（不拥有） |
| 引用计数 | 无 | 有（原子操作） | 不增加计数 |
| 拷贝 | ❌ 禁止 | ✅ 计数值+1 | ✅ 计数值不变 |
| 移动 | ✅ 所有权转移 | ✅ 性能优化 | ✅ |
| 自定义删除器 | ✅（影响类型） | ✅（不影响类型） | ❌ |
| 大小（x86_64） | 8 字节 | 16 字节 | 16 字节 |
| 场景 | 工厂、类成员、容器 | 共享缓存、DAG | 打破循环、观察者 |

#### 选型原则

```cpp
// 1. 默认用 unique_ptr —— 性能最好，语义清晰
auto p = std::make_unique<Foo>();

// 2. 明确需要共享时才用 shared_ptr
auto p = std::make_shared<Foo>();
cache["key"] = p;         // 缓存和调用者共享

// 3. 需要观察但不影响生命周期时用 weak_ptr
std::weak_ptr<Foo> w = p; // 不增加计数
```

---

### 4.6 常见陷阱

#### 陷阱 1：裸 `new` + 智能指针混用

```cpp
// ❌ 危险
Foo *raw = new Foo();
std::shared_ptr<Foo> p1(raw);
std::shared_ptr<Foo> p2(raw);  // ❌ 两个独立控制块，double free！

// ✅ 正确
auto p1 = std::make_shared<Foo>();
std::shared_ptr<Foo> p2(p1);   // 同一个控制块
```

#### 陷阱 2：`shared_ptr` 循环引用

```cpp
// 前面已讲，用 weak_ptr 打破
```

#### 陷阱 3：`this` 裸指针被智能指针管理

```cpp
struct Bad {
    std::shared_ptr<Bad> get() {
        return std::shared_ptr<Bad>(this);  // ❌ 又一个独立控制块！
    }
};
auto p = std::make_shared<Bad>();
auto q = p->get();   // ❌ double free！
// ✅ 改用 enable_shared_from_this
```

#### 陷阱 4：`unique_ptr` 作为函数参数

```cpp
// ❌ 不必要地转移所有权
void take(std::unique_ptr<Foo> p) { ... }
auto p = std::make_unique<Foo>();
take(std::move(p));   // p 被置空，调用方失去了所有权

// ✅ 如果只是想借用，传原始指针或引用
void use(Foo *p) { ... }
use(p.get());         // p 保留所有权

// ✅ 如果确实要转移所有权，用 std::move 显式表明意图
```

---

## 五、容器与内存自动管理

C++ 标准库容器是 RAII 的最好体现。它们自己管理内存，你只管放数据。

```cpp
// 不用算大小、不用记得释放
std::vector<int> v;           // 初始空
v.push_back(10);              // vector 内部自动 realloc
v.push_back(20);
v.push_back(30);              // 容量不够？vector 自动申请更大的空间
// 不用 free，出作用域自动释放

// 对比 C 风格：
int *arr = (int*)malloc(3 * sizeof(int));
arr[0] = 10; arr[1] = 20; arr[2] = 30;
int *tmp = (int*)realloc(arr, 5 * sizeof(int));  // 手动扩展
if (tmp) arr = tmp;
// 还要记得 free(arr)
```

---

## 六、C 风格 vs C++ 风格总结

| 维度 | C 风格 | C++ 风格 |
|------|--------|---------|
| 分配/释放 | `malloc/free` | `new/delete`、`make_unique`、容器 |
| 管理方式 | 手动（人记） | RAII（编译器记） |
| 失败处理 | 返回 NULL | 抛异常 `std::bad_alloc` |
| 内存所有权 | 没有概念，谁拿到谁管 | `unique_ptr` 独占、`shared_ptr` 共享 |
| 数组 | 指针+长度分开管理 | `std::vector`、`std::array` |
| 字符串 | `char*` + 手动管理终止符 | `std::string`（自动管理） |
| 泄漏风险 | 高 | 极低（智能指针 + RAII） |
| 性能开销 | 最小 | 零开销抽象（不用为不用的特性付费） |

### 一句话总结

> **C 风格**：你问操作系统要内存，自己记着还。
> **C++ 风格**：你告诉对象"你需要多少内存"，对象自己管生管死。

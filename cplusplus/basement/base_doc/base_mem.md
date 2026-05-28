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

### ptmalloc 是什么？

`ptmalloc`（Per-Thread malloc）是 glibc 中 `malloc/free` 的**具体实现**。你在 Linux 上调 `malloc(64)`，背后实际执行的就是 ptmalloc 的代码。

```bash
你写的代码                 底层实现
─────────────────────────────────────────────────
malloc(64)    ──►  ptmalloc 的 malloc() 函数
free(p)       ──►  ptmalloc 的 free() 函数
```

ptmalloc 是**目前 Linux 上最主流的 malloc 实现**。它的前身是 dlmalloc（Doug Lea malloc），后来加了多线程支持（所以叫 ptmalloc）。其他常见的 malloc 实现还有：

| 实现 | 所属 | 特点 |
|------|------|------|
| **ptmalloc** | glibc（Linux 默认） | 通用、多线程支持好、应用最广 |
| tcmalloc | Google | 线程缓存优先，小对象快 |
| jemalloc | FreeBSD / Facebook | 多线程下碎片少，常用于数据库 |
| mimalloc | Microsoft | 追求极致性能，现代设计 |

> 虽然实现不同，但它们对外都遵循 `malloc/free/realloc/calloc` 这套标准接口。所以你的代码不需要改，换一个 libc 或链接不同的分配器就能体验不同性能。下面都以 **ptmalloc** 为例讲解。

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

---

## 补充：内存池（Memory Pool）的概念

### 为什么需要内存池？

`malloc/free` 是**通用分配器**，什么大小、什么场景都能应付。但通用意味着有开销：

- 每次 malloc 都要查链表 / bin / Top Chunk
- 频繁分配释放会产生**内存碎片**
- 小块分配有元数据开销（每个 chunk 头部要占 16 字节）

如果你**只需要反复分配/释放固定大小的对象**（比如游戏中的子弹、粒子、网络连接对象），通用分配器就太重了。**内存池**就是专门解决这个问题的。

### 基本原理

```bash
传统方式（每次 malloc/free）:
    分配        释放       分配        释放
    │           │          │           │
    ▼           ▼          ▼           ▼
   [obj]  →  [空闲]  →  [obj]  →  [空闲]  ...
   每次都要查链表、可能触发系统调用

内存池方式:
    ┌──────────────────────────────┐
    │    预分配一大块内存 (pool)    │
    │  ┌──┬──┬──┬──┬──┬──┬──┬──┐   │
    │  │  │  │  │  │  │  │  │  │   │  ← 切成固定大小的小块
    │  └──┴──┴──┴──┴──┴──┴──┴──┘   │
    └──────────────────────────────┘
          ↑           ↑
      拿走一块用    用完放回来
      不需要 malloc  不需要 free
```

### 核心操作：分配与归还

#### 分配：从空闲链表头部取一个

```bash
分配前空闲链表:  [块0] → [块1] → [块2] → ... → [块N]
                        ↑
                      头指针 (指向第一个空闲块)

allocate():
  ① 取头指针指向的块
  ② 头指针移到下一个 → [块1]
  ③ 返回 [块0] 给调用者

O(1)，三步完成，不查表不遍历
```

#### 归还：插回空闲链表头部

```bash
归还前空闲链表:  [块1] → [块2] → ... → [块N]
                  ↑
                头指针

deallocate(p)  // p 指向归还的块:
  ① 把 p 的 next 指向当前头指针 → [块1]
  ② 头指针指向 p
  ③ 完成

归还后空闲链表:  [归还块] → [块1] → [块2] → ... → [块N]
                  ↑
                头指针

O(1)，三步完成，不合并不遍历
```

> 注意：内存池不检查你是否归还了"属于池里的块"。如果你 `deallocate` 一个不是从池里拿的地址，链表会被破坏——这是使用内存池需要自己保证的。

#### 实际常用的实现：嵌入空闲链表

上面用了单独的 `free_list_` 数组存指针，但更常见的做法是**在空闲块内部直接存 next 指针**（块空闲时，这块内存本身就是空闲的，拿它的前 8 字节存下一个地址完全不影响）：

```cpp
// 嵌入式空闲链表：不额外占用内存
union Block {
    T data;                     // 当块被分配时，用来存真正的对象
    Block* next;                // 当块空闲时，前 8 字节存下一个空闲块的地址
};

class EmbeddedPool {
    Block* free_head_;          // 空闲链表头指针
    char* pool_mem_;            // 整块池内存
    
public:
    EmbeddedPool(size_t n) {
        pool_mem_ = new char[sizeof(Block) * n];
        // 初始化空闲链表：每个空闲块的 next 指向下一个块
        free_head_ = (Block*)pool_mem_;
        Block* cur = free_head_;
        for (size_t i = 0; i < n - 1; i++) {
            cur->next = (Block*)(pool_mem_ + sizeof(Block) * (i + 1));
            cur = cur->next;
        }
        cur->next = nullptr;           // 最后一个块 next = null
    }
    
    T* allocate() {
        if (!free_head_) return nullptr;
        Block* b = free_head_;
        free_head_ = free_head_->next; // 头指针移到下一个
        return &b->data;               // 返回给用户（作为 T* 用）
    }
    
    void deallocate(T* p) {
        Block* b = (Block*)p;
        b->next = free_head_;          // 归还块指向当前头
        free_head_ = b;                // 头指针指向归还块
    }
    
    ~EmbeddedPool() { delete[] pool_mem_; }
};
```

```bash
嵌入式空闲链表的内存布局:

初始化后:
┌──────┬──────┬──────┬──────┬──────┐
│ next→│ next→│ next→│ next→│ null │
│ ──── │ ──── │ ──── │ ──── │      │
└──┴───└──┴───└──┴───└──┴───└──────┘
   ↑
free_head_

分配两块后:
┌──────┬──────┬──────┬──────┬──────┐
│ objA │ objB │ next→│ next→│ null │
│      │      │ ──── │ ──── │      │
└──────┴──────┴──┴───└──┴───└──────┘
                  ↑
              free_head_

归还 objA 后 (注意顺序无关):
┌──────┬──────┬──────┬──────┬──────┐
│ next→│ objB │ next→│ next→│ null │  ← objA 的前8字节存next
│ ──── │      │ ──── │ ──── │      │
└──┴───└──────└──┴───└──┴───└──────┘
   ↑
free_head_  链表: [A] → [C] → [D]
```

> 这种"嵌入式"技巧利用了**块空闲时里面的数据是无用的**这一事实，拿空闲块自身的内存来存链表指针，不需要额外的指针数组。很多真实的内存池（如 tcmalloc、内核 slab 分配器）都用了类似思路。

### 池满了怎么办？

内存池的大小是固定的，如果所有块都用完了（`free_head_ == nullptr`），有三种处理策略：

```bash
策略 1: 返回 nullptr（最简单）
  allocate() → nullptr
  调用者自己处理（如用 fallback malloc）

策略 2: 挂载新池（常见做法）
  ┌────────────┐    ┌────────────┐    ┌────────────┐
  │  Pool 0    │ →  │  Pool 1    │ →  │  Pool 2    │
  │  100 blocks │    │  100 blocks│    │  100 blocks│
  └────────────┘    └────────────┘    └────────────┘
  ↑                 ↑                 ↑
  active           reserve           reserve
  当前池满了 → 切到下一个池 → 还不够 → 再 new 一个 Pool 挂链表尾部

策略 3: 回退到 malloc（保底）
  if (pool 空) return malloc(sizeof(T));
  deallocate 时如果发现是 pool 外来的，直接用 free(p)
```

### 池的创建与销毁

```cpp
// 创建内存池
// ① 确定块大小（sizeof(T)，对齐到 alignof(T)）
// ② 确定块数量（根据场景预估峰值
// ③ 分配 pool_mem_ = new char[sizeof(Block) * N]
// ④ 初始化空闲链表

// 销毁内存池
// ① 如果对象有析构函数，需要对所有已分配对象手动调用析构
// ② 释放 pool_mem_ = delete[] pool_mem_
// ⚠️ 注意：如果池没销毁而对象还在用，对象持有的指针全是野指针

// 一个更完整的示例
template<typename T>
class Pool {
    union Block {
        T data;
        Block* next;
    };
    Block* free_head_;
    char* pool_mem_;            // 对于char*变量 pool_mem_ + 1实际上地址只多了1B(int * + 1 -> + 4B)
                                // 当需要按任意偏移量切割内存时（例如计算第n个 Block 的地址），只有 char* 能给出精确的字节级寻址
    size_t capacity_;
    size_t used_ = 0;           // 记录当前已分配了多少块
    
public:
    Pool(size_t n) : capacity_(n) {
        pool_mem_ = new char[sizeof(Block) * n];
        free_head_ = (Block*)pool_mem_;
        Block* cur = free_head_;
        for (size_t i = 0; i < n - 1; i++) {
            cur->next = (Block*)(pool_mem_ + sizeof(Block) * (i + 1));
            cur = cur->next;
        }
        cur->next = nullptr;
    }
    
    T* allocate() {
        if (!free_head_) return nullptr;   // 池空
        used_++;
        Block* b = free_head_;
        free_head_ = free_head_->next;
        return &b->data;
    }
    
    void deallocate(T* p) {
        // 检查 p 是否属于这个池（地址在 pool_mem_ ~ pool_mem_ + 容量之间）
        if ((char*)p < pool_mem_ || (char*)p >= pool_mem_ + capacity_ * sizeof(Block))
            return;  // 不是本池的内存，不做处理
        used_--;
        Block* b = (Block*)p;
        b->next = free_head_;
        free_head_ = b;
    }
    
    ~Pool() {
        // 如果 used_ > 0，说明还有对象在使用池内存
        // 正常情况下应由调用者保证所有对象已归还
        delete[] pool_mem_;
    }
};
```

### 内存池 vs ptmalloc 对比

| 特性 | ptmalloc（通用分配器） | 内存池 |
|------|---------------------|--------|
| 分配速度 | O(log n) ~ O(n) 查链表 | **O(1)** 直接取 |
| 释放速度 | 可能合并、可能缩堆 | **O(1)** 直接归还 |
| 外部碎片 | 容易产生 | 零碎片（固定大小） |
| 元数据开销 | 每块约 16 字节头部 | **几乎为 0**（空闲时用嵌入指针） |
| 适用场景 | 任意大小、任意模式 | **固定大小、频繁分配释放** |
| 灵活性 | 高 | 低（只适合特定大小） |
| 线程安全 | 内置支持 | 需自己加锁 |
| 池满处理 | 自动向内核扩展 | 需手动处理或回退 |

### 实际中的内存池

- **C++ std::allocator**：很多 STL 实现的默认分配器内部对固定大小（如 list 节点）做了池化
- **游戏引擎**：每帧大量创建/销毁的对象（粒子、子弹）几乎必用内存池
- **网络库**：连接对象、缓冲区用内存池管理
- **Linux 内核 Slab 分配器**：本质就是个内存池——为每种内核对象（inode、dentry）维护专用缓存

> **一句话总结**：内存池就是提前借一笔钱放兜里，每次花的时候直接从兜里掏，不用每次都去银行（内核）取。

---

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

> ⚠️ **重要**：`new[]` 必须配 `delete[]`，`new` 配 `delete`。混用是未定义行为。

#### 是否需要记录数组长度？—— 看类型

`new[]` 是否在内存头部存数组长度，取决于元素类型**有没有非平凡析构函数**：

```bash
new[3] 的两种行为:

情况 A: 元素是 int / double / POD结构体（纯旧式数据完全和C的结构体等价），没有析构函数
  ┌──────────┬──────────┬──────────┐
  │ arr[0]   │ arr[1]   │ arr[2]   │
  └──────────┴──────────┴──────────┘
  ▲
  └── new 返回的地址
  delete[] 只需释放整块内存，不需要调用任何析构，所以不需要存长度 ✅

情况 B: 元素是 std::string / 有析构函数的类
  ┌──────┬──────────┬──────────┬──────────┐
  │ [3]  │ arr[0]   │ arr[1]   │ arr[2]   │  ← 需要逐个调用析构
  └──────┴──────────┴──────────┴──────────┘
  ▲      ▲
  │      └── new 返回的地址（程序员看到的）
  └── 隐藏的数组长度（delete[] 用来确定析构次数）
```

这就是为什么：

```cpp
// 对 int 数组，delete 和 delete[] 可能"碰巧都能运行"
// ——但 delete 依然是未定义行为，不要这样写！
int *a = new int[10];
delete a;      // ❌ 未定义行为（虽然可能不崩，因为不需要析构）
delete[] a;    // ✅ 正确

// 对有析构函数的类型，delete 没加 [] 一定出问题
std::string *b = new std::string[10];
delete b;      // ❌ 只析构了 b[0]，剩下 9 个泄漏
delete[] b;    // ✅ 正确
```

> **规则**：`new[]` 永远配 `delete[]`，不管什么类型。编译器会根据类型自动选择要不要存长度。

### 1.4 Placement New —— 在已存在的内存上构造对象

`new` 通常做两件事：① 分配内存 ② 调用构造函数。但有时候你已经有了一块内存（比如从内存池拿的、从共享内存获得的），只想**在这块现成的内存上调用构造函数**——这就是 placement new。

```cpp
#include <new>    // placement new 需要这个头文件

void* mem = malloc(sizeof(Stu));      // 只分配内存，不构造
Stu* p   = new(mem) Stu("Jason", 23); // ✅ 在 mem 这块内存上构造 Stu
// 此时 p == mem，p->name_ = "Jason", p->age_ = 23

// 用完：手动调用析构，用 free 释放内存
p->~Stu();                            // ✅ 析构对象
free(mem);                            // 释放内存
// ⚠️ 不能 delete p！delete 会尝试释放 p 指向的内存，
//    但内存不是你用 new 分配的，释放行为未定义
```

#### 用在模拟内存池中

```cpp
// 从内存池拿一块裸内存
void* mem = pool.allocate();

// 在这块内存上构造对象（placement new）
auto obj = new(mem) Stu("Jason", 23);

// 使用 obj ...
obj->print();

// 手动析构（内存池不负责析构）
obj->~Stu();

// 归还裸内存给池
pool.deallocate(mem);
```

#### 和普通 `new` 对比

```bash
普通 new:               Stu* p = new Stu("Jason", 23);
                        ① malloc(sizeof(Stu))  ← 分配内存
                        ② Stu::Stu("Jason",23) ← 构造对象

Placement new:          void* mem = malloc(sizeof(Stu));
                        Stu* p = new(mem) Stu("Jason", 23);
                        ① 跳过分配，直接用 mem 这块地址
                        ② Stu::Stu("Jason",23) ← 只构造，不分配

释放对比:
普通 new →  delete p;       // ① 析构 ② free
Placement new → p->~Stu();   // ① 只析构（内存谁分配谁释放）
```

> **本质**：placement new 不是"分配内存"，它只是一个**语法糖**，让你在指定地址上调用构造函数。对应的"释放"也不是 `delete`，而是手动调用析构函数，然后由分配内存的那一方去释放。

### 1.5 `new` 失败怎么办？⭐

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
int *p = new (std::nothrow) int[100]; // 表明构造int[100]时，如果有异常不报错 p = nullptr返回空指针
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

有时，默认的 delete 操作不适用于所有资源管理场景。此时，可以使用自定义删除器来指定资源释放的方式。例如，管理文件句柄、网络资源或自定义清理逻辑。

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

#### `make_shared` 用法

`make_shared` 是创建 `shared_ptr` 的首选方式（C++11 引入），它在一次堆分配中同时创建对象和控制块。

##### 基本语法

```cpp
#include <memory>

// 创建基本类型
auto p1 = std::make_shared<int>(42);
// 类型: shared_ptr<int>，值为 42

// 创建对象（完美转发参数）
auto p2 = std::make_shared<Foo>(arg1, arg2);
// 等价于 shared_ptr<Foo>(new Foo(arg1, arg2))，但更高效

// 创建数组 (C++20)
auto p3 = std::make_shared<int[]>(10);  // 10 个 int，值默认初始化
// C++17 及以前推荐用 vector 代替
```

##### 常用场景

```cpp
// ① 简单创建
auto sp = std::make_shared<std::string>("hello");
std::cout << *sp << '\n';          // 解引用
std::cout << sp->size() << '\n';   // 调成员函数

// ② 配合容器
std::vector<std::shared_ptr<Foo>> vec;
vec.push_back(std::make_shared<Foo>(1, 2));  // ✅ 可以直接 push
// 不需要 move，shared_ptr 可以拷贝

// ③ 作为函数返回值
std::shared_ptr<Foo> create() {
    return std::make_shared<Foo>(10, 20);  // 自动移动或拷贝
}

// ④ 配合 weak_ptr 观察
std::weak_ptr<Foo> w = std::make_shared<Foo>();
if (auto sp = w.lock()) {
    sp->method();  // 对象还在
}
```

##### 什么时候不能用 `make_shared`？

```cpp
// ① 需要自定义删除器时，只能用 shared_ptr 构造函数
std::shared_ptr<FILE> p(fopen("a.txt", "r"), fclose);
// make_shared 不支持自定义删除器

// ② 需要控制对象和控制块的生命周期分离时（见下方原理）
// 这时用 shared_ptr(new T())，但需要承担两次分配的开销
```

##### 和 `shared_ptr<T>(new T())` 对比

| 对比项 | `make_shared` | `shared_ptr<T>(new T())` |
|-------|--------------|------------------------|
| 分配次数 | **1 次**（对象+控制块一起） | 2 次（对象和控制块分开） |
| 内存碎片 | 更少 | 可能更多 |
| 缓存局部性 | 更好（紧挨着） | 较差 |
| 异常安全 | ✅ 安全 | ❌ 可能泄漏（new 后构造 shared_ptr 前抛异常） |
| 自定义删除器 | ❌ 不支持 | ✅ 支持 |
| 对象内存释放时机 | 等 weak 也归零 | 强引用归零立即释放 |

##### 选型建议

```cpp
// ✅ 默认用 make_shared（快、安全）
auto p = std::make_shared<Foo>(args...);

// ✅ 需要自定义删除器时
std::shared_ptr<Foo> p(new Foo(args...), custom_deleter);

// ✅ 需要精确控制对象内存释放时机（不让 weak 拖累对象内存）
std::shared_ptr<Foo> p(new Foo(args...));
```

#### 原理：为什么 `shared_ptr` 比 `unique_ptr` 多占一倍内存？

##### 核心矛盾

`unique_ptr` 是独占的——它销毁时可以直接 `delete` 对象，不用担心别人还在用。

`shared_ptr` 是共享的——多个 `shared_ptr` 指向同一个对象，**最后一个销毁的才负责释放**。但每个 `shared_ptr` 是独立创建和销毁的，它们之间怎么协调？

```cpp
auto p1 = std::make_shared<Foo>();   // p1 创建
auto p2 = p1;                        // p2 和 p1 指向同一个 Foo
// p2 先析构 → 怎么知道 Foo 不能释放？（因为 p1 还在用）
// p1 析构   → 怎么知道可以释放了？（因为只有 p1 了）
```

答案是需要一个**所有 `shared_ptr` 都能读写的计数器**。

##### 计数器放哪？

```bash
❌ 放在 Foo 对象内部？
   Foo 是用户定义的类（如 struct Foo { int a; };），它不知道也不关心
   自己是不是被 shared_ptr 管理，没有位置存计数器。

❌ 放在某个 shared_ptr 内部？
   每个 shared_ptr 是独立的变量，一个 shared_ptr 看不到另一个内部的数据。

✅ 放在堆上，单独分配一块内存来存计数器？
   可行！所有 shared_ptr 都通过指针访问这块内存，大家都能读写。
   这块额外分配的内存就叫——控制块（Control Block）。
```

控制块里至少存一个东西：**引用计数**（refcount）——当前有多少个 `shared_ptr` 指向同一个对象。

##### 现在能说清 `shared_ptr` 为什么是 16 字节了

`shared_ptr` 栈对象内部存了两个指针：

```bash
                 每个 shared_ptr<Foo> 栈对象（16 字节）

┌──────────────────────┐  ┌──────────────────────┐
│  ptr: T*              │  │  ctrl: ControlBlock*  │
│  指向真正的 Foo 对象    │  │  指向堆上的控制块     │
│  （用于快速解引用）     │  │                      │
│  8 字节               │  │  8 字节               │
└──────────────────────┘  └──────────────────────┘
```

- **`ptr`**：指向真正的对象，你调用 `sp->method()`、`*sp`、`sp.get()` 时直接读这个指针，**不需要去堆上查控制块**，所以解引用性能和裸指针一样快。
- **`ctrl`**：指向堆上的控制块（存引用计数等信息），用于判断何时释放。

#### 控制块里有什么？

控制块是一个**多态基类**——不同的构造方式派生出不同的控制块子类，通过虚函数实现类型擦除的销毁逻辑。

##### 为什么控制块需要 vtable？

控制块有虚函数表（vtable），是因为 **"销毁对象"的方式对控制块来说是未知的**：

| 构造方式 | 需要什么销毁操作 | 对应的控制块子类 |
|---------|----------------|----------------|
| `new Foo` + `shared_ptr` | `delete ptr` | `_Sp_counted_ptr<Foo>` |
| `make_shared<Foo>()` | 析构内嵌对象 → 释放整块内存 | `_Sp_counted_ptr_inplace<Foo>` |
| 自定义删除器 | `my_deleter(ptr)` | `_Sp_counted_deleter<Foo, Deleter>` |

控制块通过虚函数 `dispose()` / `destroy()` 来执行正确的销毁逻辑。当引用计数归零时：

```cpp
// 控制块内部（伪代码）
void release() {
    if (--refcount == 0) {
        dispose();      // 虚函数调用 → 不同类型的控制块执行不同的销毁
        if (--weakcount == 0)
            destroy();  // 虚函数调用 → 释放自身
    }
}

// _Sp_counted_ptr 的 dispose：
void dispose() override { delete ptr; }

// _Sp_counted_ptr_inplace 的 dispose：
void dispose() override { storage->~T(); }
```

**这就是类型擦除**：`shared_ptr<T>` 不需要知道 `T` 的具体删除方式，虚函数把这一切隐藏在了 vtable 里。

##### 控制块内部结构

```bash
控制块内部结构（libstdc++ 实测）:

┌────────────────────────────────────────┐
│  控制块头部 (Control Block Header)      │
│  ┌──────────┬──────────┬──────────────┐ │
│  │ vtable   │ refcount │ weakcount    │ │
│  │ (8字节)  │ (4字节)  │ (4字节)      │ │
│  │ 指向虚表 │ 强引用计数│ 弱引用计数   │ │
│  │          │ int      │ int          │ │
│  └──────────┴──────────┴──────────────┘ │
│   总大小：libstdc++ 上 16 字节            │
│   （不同标准库实现可能不同）               │
├────────────────────────────────────────┤
│  对象存储区域                            │
│  ┌────────────────────────────────────┐ │
│  │ 取决于构造方式:                      │ │
│  │ • make_shared → Foo 对象内嵌在此    │ │
│  │ • new方式     → 指向 Foo 的指针    │ │
│  └────────────────────────────────────┘ │
└────────────────────────────────────────┘
```

> 引用计数的大小：**libstdc++ 上 `_Atomic_word` 是 `int`（4 字节）**，GDB 实测 `_M_use_count` 和 `_M_weak_count` 各 4 字节。不同标准库（MSVC STL 等）可能使用 8 字节的原子类型，控制块总大小会有所不同。
> refcount是表示多少个shared_ptr指向对象，weakcount表示有多少个weak_ptr指向对象（不增加强引用计数）

##### 控制块自己存一份指针

关键：**控制块里也存了指向对象的指针（或者对象本身）**。

为什么？因为引用计数归零时，**删除操作是在控制块的析构逻辑中触发的**，而不是由栈上的 `shared_ptr` 触发的：

```cpp
// 伪代码：当 refcount 从 1 → 0 时，控制块内部做：
this->ptr_to_object->~T();   // 析构对象
operator delete(this->ptr_to_object);  // 释放内存
```

如果控制块不保存指向对象的指针，它就没法知道该 delete 谁。

#### 两种构造方式，两种布局

##### 方式 A：`make_shared` —— 一次分配，对象内嵌

```cpp
auto p = std::make_shared<Foo>();
```

```bash
堆上一次性 malloc:

┌──────────────┬──────────────────────┐
│  控制块头部    │  Foo 对象            │
│ (v+ref+weak) │  (内嵌存储)           │
│   16 字节     │   sizeof(Foo)        │
└──────────────┴──────────────────────┘
▲               ▲
│               └── p.ptr 指向这里
└── p.ctrl 指向这里
```

优点：**一次分配**，对象和控制块在一起，缓存友好。
缺点：当所有 `shared_ptr` 销毁后，**对象的析构函数会执行**，但**对象占用的内存不会归还给系统**。这块内存要等到 `weak_ptr` 也全部销毁、控制块自身释放时才一并回收。如果对象持有大块内存（如 `vector<char>(100MB)`），即使所有 `shared_ptr` 都不用了，只要还有一个 `weak_ptr` 存在，这 100MB 就无法被复用，可能造成内存峰值。

##### 方式 B：`new` + `shared_ptr` —— 两次分配，控制块存指针

```cpp
std::shared_ptr<Foo> p(new Foo());
```

```bash
第一次分配 (new Foo):
┌──────────────────────┐
│   Foo 对象            │  ← 独立分配
└──────────────────────┘

第二次分配 (shared_ptr 构造):
┌──────────────┬──────────────────────┐
│  控制块头部    │  ptr: 指向 Foo 的指针│  ← 控制块自己存一份指针
│ (v+ref+weak) │  (8 字节)            │
│   16 字节     │                      │
└──────────────┴──────────────────────┘
```

```cpp
// 控制块内部的伪代码
template<typename T>
struct Sp_counted_ptr {
    long refcount;   // 引用计数
    long weakcount;  // 弱计数
    T* ptr;          // ⭐ 控制块自己存一份指针！
    
    void dispose() {
        delete ptr;  // 引用计数归零时，控制块通过自己的 ptr 删除对象
    }
};
```

优点：对象和控制块解耦，`weak_ptr` 不影响对象内存释放。
缺点：**两次分配**，地址不一定连续，缓存局部性差。

#### 为什么控制块要自己存指针，不能直接用栈上 `shared_ptr` 的 `ptr`？

因为当最后一个 `shared_ptr` 销毁时：

```cpp
auto p1 = std::make_shared<Foo>();
auto p2 = p1;

// p1 先析构：
//   p1 的析构函数调用 ctrl->release()
//   控制块计数从 2 → 1，不释放

// p2 析构：
//   p2 的析构函数调用 ctrl->release()
//   控制块计数从 1 → 0
//   控制块内部调用 delete(自己存的指针)  ← 此时 p2 已经销毁了！
```

当"删除对象"这个动作执行时，所有的 `shared_ptr` 栈对象都已经销毁了。只有控制块还在堆上活着，所以**只能由控制块来执行删除**，控制块必须自己存着指向对象的指针。

#### 实测：验证两种布局

```cpp
// test: make_shared
auto p1 = std::make_shared<Foo>();
cout << "p1.ptr  = " << p1.get() << endl;   // 0x...
// 控制块在 p1.ptr 前面，紧挨着

// test: new + shared_ptr
auto p2 = std::shared_ptr<Foo>(new Foo());
cout << "p2.ptr  = " << p2.get() << endl;   // 0x...
// 控制块在另一个地址，和 p2.ptr 地址无固定关系
// 因为控制块里的 ptr 和 p2 的 ptr 是同一个地址的两次拷贝
```

#### 控制块大小实测

```cpp
auto p = std::make_shared<Foo>();
auto raw = (char*)p.get();

void* vtable = nullptr;
memcpy(&vtable, raw - 16, 8);    // Foo 前 16 字节处 → vtable 指针 ✅
// vtable = 0x6179f82cbc80  ← 指向代码段的有效地址

void* not_vtable = nullptr;
memcpy(&not_vtable, raw - 24, 8); // Foo 前 24 字节处 → 0x61 ❌ 不是指针
memcpy(&not_vtable, raw - 32, 8); // Foo 前 32 字节处 → 0    ❌
```

控制块头部（vtable + refcount + weakcount）在 **libstdc++ 上实测 16 字节**。不同标准库实现可能不同（如 MSVC STL 的引用计数可能使用 8 字节原子类型）。对于 `make_shared`，对象紧跟在控制块后面；对于 `new` 方式，控制块后面跟的是一个 8 字节指针，指向独立分配的对象。

##### 完整生命周期

```cpp
auto p1 = std::make_shared<Foo>();
// make_shared 内部:
//   ① malloc 一块内存: |-- 控制块{vptr+ref+weak} --|--- Foo ---|
//   ② 在 Foo 位置上调用 Foo() 构造函数
// p1.ptr  → Foo 位置   (用于快速解引用)
// p1.ctrl → 控制块位置 (用于计数)

{
    auto p2 = p1;
    // p2.ptr  = p1.ptr  (指向同一个 Foo)
    // p2.ctrl = p1.ctrl (指向同一个控制块)
    // 控制块.refcount = 2   (拷贝构造时 +1)
    // ... 用 Foo ...
}
// p2 析构:
//   控制块.refcount = 1   (析构时 -1)
//   计数 > 0 → 不释放

// p1 析构:
//   控制块.refcount = 0   (析构时 -1)
//   计数 == 0 → 控制块调用 delete/析构，释放整块内存
```

##### 如果不用 `make_shared`，用 `new` + `shared_ptr` 呢？

```cpp
std::shared_ptr<Foo> p(new Foo());
// 此时堆上是两次独立分配:
//   [Foo 对象]         ← new Foo()
//   [控制块{计数=1}]   ← shared_ptr 构造时内部再分配
// 两个内存块不一定是连续的，缓存局部性差一些
```

> **对比**：`make_shared` 一次分配，对象和控制块紧挨着，更快、碎片更少。缺点是在所有 `weak_ptr` 销毁之前，即使对象已经没 `shared_ptr` 用了，整块内存（包括对象占的空间）也不能释放（因为控制块还在）。但通常这不是问题。

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

#### 循环引用问题

当两个对象用 `shared_ptr` 互相指向对方时，引用计数永远降不到 0，导致内存泄漏。

先理清例子中的变量：

```cpp
struct Node {
    std::shared_ptr<Node> next;  // Node 内部有个 shared_ptr 成员
};

auto a = std::make_shared<Node>();  // a 是 shared_ptr<Node>（栈上）
auto b = std::make_shared<Node>();  // b 是 shared_ptr<Node>（栈上）

// a->next 等价于 (*a).next，访问 Node 对象的 next 成员
// a->next = b  意思是：让 a 内部的 shared_ptr 指向 b 所指向的 Node 对象
// 结果：a 和 b 两个 Node 对象的 next 成员互相指向对方
a->next = b;  // a 的 next 指向 b 管理的 Node
b->next = a;  // b 的 next 指向 a 管理的 Node
```

##### 为什么计数不会归零？

```bash
① a = make_shared<Node>()    →  a 的控制块 { refcount = 1 }
② b = make_shared<Node>()    →  b 的控制块 { refcount = 1 }

③ a->next = b  →  b 内部又多了一个 shared_ptr 指向它
                    b 的控制块 { refcount = 2 }   ← a.next 也持有 b

④ b->next = a  →  a 内部也多了一个 shared_ptr 指向它
                    a 的控制块 { refcount = 2 }   ← b.next 也持有 a

⑤ a 析构       →  a 的控制块 { refcount = 1 }   ← 还有 b.next 指着 a
⑥ b 析构       →  b 的控制块 { refcount = 1 }   ← 还有 a.next 指着 b

⑦ 没人了！两个控制块的计数都卡在 1，谁都没法释放谁
```

```bash
循环引用图解:

     a (栈上) ──────────→  a 控制块{ref=1}
        │
        │                    ▲
        └── a.next ──→  b ──┘
                        │
                        └── b.next ──→  a ──→  a 控制块{ref=1}
                                            ▲
     b (栈上) ──────────→  b 控制块{ref=1} ──┘


→ 出作用域时，a 和 b 栈对象析构，分别 -1
→ a 控制块: ref=1（被 b.next 指着）→ 泄漏
→ b 控制块: ref=1（被 a.next 指着）→ 泄漏
```

##### 典型的循环引用场景

```cpp
// 场景 1：双向链表 / 二叉树（父子相互指向）
struct TreeNode {
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
    std::shared_ptr<TreeNode> parent;  // ❌ 父指向子，子指向父 → 循环
};

// 场景 2：观察者模式（subject 和 observer 互相持有）
struct Observer;
struct Subject {
    std::vector<std::shared_ptr<Observer>> observers;  // ❌
};
struct Observer {
    std::shared_ptr<Subject> subject;  // ❌
};
```

##### 解决办法：用 `weak_ptr` 打破其中一个方向

```cpp
struct Node {
    std::weak_ptr<Node> next;  // ✅ 不增加引用计数
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->next = a;
// a 控制块 refcount = 1（只有 a 自己持有）
// b 控制块 refcount = 1（只有 b 自己持有）
// 出作用域自动释放 ✅
```

> **规则**：在双向引用中，把**非所有权方向**（如 parent、观察者、缓存）设为 `weak_ptr`。谁"拥有"谁，就用 `shared_ptr`；谁"观察"谁，就用 `weak_ptr`。

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

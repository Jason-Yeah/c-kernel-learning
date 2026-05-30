# STL 标准模板库

STL = Standard Template Library，C++ 标准库的核心组成部分。它不是"一个库"，而是**六大组件**的有机组合：
容器（Containers）、算法（Algorithms）、迭代器（Iterators）、仿函数（Functors）、适配器（Adapters）、空间配置器（Allocators）

---

# 一、六大组件总览

```bash
容器          ← 存储数据
    ↓
迭代器        ← 容器与算法的桥梁（每种容器提供自己的迭代器）
    ↓
算法          ← 通过迭代器操作容器（sort, find, for_each...）
    ↑
仿函数        ← 算法的策略参数（less<int>, greater<int>...）
    ↑
适配器        ← 接口转换（stack 适配 deque, queue 适配 deque...）
    ↑
空间配置器    ← 底层内存分配（所有容器的基石）
```

---

# 二、空间配置器（Allocator）—— 一切的基础

容器的底层不直接调用 `new`/`malloc`，而是通过分配器（allocator）获取内存。

## 默认分配器 std::allocator

```cpp
#include <memory>

std::allocator<int> alloc;
int* p = alloc.allocate(1);   // 分配 1 个 int 的空间，但不构造
alloc.construct(p, 42);       // 在 p 处构造 int(42)
std::cout << *p << std::endl; // 42
alloc.destroy(p);             // 析构
alloc.deallocate(p, 1);       // 释放
```

## GCC 两级分配器（__gnu_cxx::__pool_alloc）

```cpp
/* 源码：libstdc++ 默认使用 __mt_alloc 或 new_allocator
 * 但历史上 SGI STL 的两级分配器是教科书级别经典设计 */

// 一级分配器（>128 字节）：直接调用 malloc
static void* allocate(size_t __n) {
    void* __ret = ::operator new(__n); // → malloc
    return __ret;
}

// 二级分配器（≤128 字节）：内存池 + 自由链表
// 维护 16 条自由链表（free_list），每条管理一个固定大小块：
// 8, 16, 24, 32, ..., 128 字节
union _Obj {
    union _Obj* _M_free_list_link;  // 指向下一个空闲块（嵌入指针！）
    char _M_client_data[1];         /* 用户数据起始 */
};

static _Obj* volatile _S_free_list[_S_max_bytes + _S_align - 1];

// 分配：从对应链表头部取一块
static void* allocate(size_t __n) {
    if (__n > 128) return __malloc_alloc::allocate(__n);
    _Obj* __restrict__ __result = _S_free_list[_S_round_up(__n) / _S_align - 1];
    if (__result == nullptr) _S_refill(_S_round_up(__n)); // 链表为空，从内存池取
    _S_free_list[...] = __result->_M_free_list_link;      // 头部弹出
    return __result;
}
```

### 关键内存优化：嵌入空闲链表（Embedded Free List）

二级分配器**不额外占用内存**来存储链表指针——指针直接存在**空闲块本身**的前 4/8 字节里。

```text
分配前（块在被使用中）：
┌──────────────┐
│  用户数据...  │   ← 8 字节对齐
└──────────────┘

释放后（块进入自由链表）：
┌──────────────┐
│  next 指针   │   ← 复用块的前 8 字节存链表指针
├──────────────┤
│  [空闲空间]   │
└──────────────┘
```

这是"已分配的内存块不需要链表指针，空闲的内存块才有"的思想——指针**不额外占用内存**。

### 内存池（Memory Pool）—— 二级分配器的水源

当某条自由链表为空时，二级分配器从**内存池**（一个大块连续内存，通过 `malloc` 获取）切割一块出来：

```
内存池（由底层 malloc 维护的一大块连续区域）：
┌────────────┬────────────┬────────────┬────────────┐
│  20 块 8B  │  16 块 16B │  ...       │  ...        │
└────────────┴────────────┴────────────┴────────────┘
    ↓ 切割      ↓ 切割
自由链表[0]   自由链表[1]
(8B 块)       (16B 块)
```

### 为什么 STL 需要自己的分配器？

1. **减少 malloc 调用次数**：小对象每次 `new` 都调 `malloc` 太慢，二级分配器批量预取
2. **减少内存碎片**：固定大小块无外部碎片
3. **避免每个小对象都带 malloc 头开销**：malloc 每个块有约 8-16 字节的管理头（chunk header），STL 二级分配器规避了它
4. **空间局部性好**：同类对象聚集在同一页，缓存友好

**注意**：现代 C++17 以后，多数实现直接用 `new_allocator`（包装 `::operator new`）作为默认，两级分配器的优化效果被更强大的 ptmalloc / tcmalloc / jemalloc 替代，但设计思想仍然经典。

---

# 三、序列式容器

## 1. vector —— 动态数组

### 基本用法

```cpp
#include <vector>

std::vector<int> v;          // 空 vector
std::vector<int> v2(5, 10);  // 5 个 10
std::vector<int> v3 = {1,2,3,4,5};

v.push_back(6);              // 尾部插入 O(1)
v.pop_back();                // 尾部删除 O(1)
v.insert(v.begin(), 0);      // 头部插入 O(n)
v.erase(v.begin() + 2);      // 删除第三个元素 O(n)
v[3] = 100;                  // 随机访问 O(1)
v.at(3) = 100;               // 带边界检查
v.size();                    // 元素个数
v.capacity();                // 当前容量（已分配空间能容纳多少元素）
v.reserve(100);              // 预分配容量
v.shrink_to_fit();           // 请求收缩容量到等于 size
```

### 底层实现——连续内存的三指针

```
/* 源码：libstdc++ bits/stl_vector.h */

template<typename _Tp, typename _Alloc>
struct _Vector_impl_data {
    _Tp* _M_start;        // 指向已使用空间的起始
    _Tp* _M_finish;       // 指向已使用空间的末尾（最后一个元素之后）
    _Tp* _M_end_of_storage; // 指向已分配空间的末尾
};
```

```text
            _M_start          _M_finish         _M_end_of_storage
              ↓                  ↓                   ↓
        ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
        │ 0  │ 1  │ 2  │ 3  │ 4  │    │    │    │    │    │
        └────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
        ├──────── 已构造元素 ────────┤├────── 预留空间 ───────┤
```

- `size() = _M_finish - _M_start`
- `capacity() = _M_end_of_storage - _M_start`

### 扩容机制（内存底层）

```cpp
/* 源码：libstdc++ bits/stl_vector.h */
void _M_realloc_insert(size_t __position, const _Tp& __val) {
    const size_t __old_size = size();
    const size_t __new_size = __old_size ? 2 * __old_size : 1;
    // ↑ GCC 扩容因子是 2（VS 是 1.5）
    
    // 从分配器获取新内存
    auto __new_start = _M_allocate(__new_size);
    auto __new_finish = __new_start;
    
    // 1. 拷贝构造已有元素到新位置
    // 2. 插入新元素
    // 3. 销毁旧元素
    // 4. 释放旧内存
}
```

```text
扩容前（capacity = 5, size = 5）：
    [_M_start]         [_M_finish/_M_end_of_storage]
    [0][1][2][3][4]
    
push_back(5) → 发现 _M_finish == _M_end_of_storage，触发扩容：

    [0][1][2][3][4]   ← 老内存（释放）
    
    新分配 capacity = 10 的连续内存：
    [0][1][2][3][4][5][ ][ ][ ][ ]
    ↑               ↑
    _M_start      _M_finish  _M_end_of_storage
```

**扩容三步走**（时间复杂度 O(n)）：

1. 新分配一块连续内存，大小为原来的 2 倍
2. 将旧元素逐个拷贝/移动到新内存（C++11 起优先 move）
3. 销毁旧元素，释放旧内存

> ⚠️ **迭代器失效**：扩容后所有指向原 vector 的迭代器、指针、引用全部失效——因为内存地址变了。

### C++11 move 优化

```cpp
/* 源码：libstdc++ 当类型提供 noexcept move 构造时 */
_GLIBCXX_MOVE_IF_NOEXCEPT_SPEC

// 分配新内存后，如果 _Tp 可 noexcept move，则 move 而非 copy
// 典型如 std::string 或 std::vector 自身
```

---

## 2. list —— 双向链表

### 基本用法

```cpp
#include <list>

std::list<int> lst = {1, 2, 3, 4, 5};
lst.push_front(0);         // 头部插入 O(1)
lst.push_back(6);          // 尾部插入 O(1)
lst.pop_front();           // 头部删除 O(1)
lst.pop_back();            // 尾部删除 O(1)
lst.insert(++lst.begin(), 99); // 在第二个位置插入 O(1)
lst.erase(lst.begin());    // 删除第一个元素 O(1)
lst.remove(3);             // 删除所有值为 3 的节点 O(n)
lst.sort();                // 归并排序 O(n log n)
lst.unique();              // 去重（相邻重复合并只剩一个）
lst.splice(lst.end(), another_lst); // 将另一个 list 拼接到尾部 O(1)

// 注意：list 没有 operator[]，不支持随机访问
```

### 底层实现——带头双向循环链表

```
/* 源码：libstdc++ bits/stl_list.h */

struct _List_node_base {
    _List_node_base* _M_next;  // 后继
    _List_node_base* _M_prev;  // 前驱
};

template<typename _Tp>
struct _List_node : public _List_node_base {
    _Tp _M_data;               // 实际数据
};

// list 本身只含一个哨兵节点指针
template<typename _Tp, typename _Alloc>
class list {
    _List_node<_Tp>* _M_node; // 指向哨兵节点（头节点）
    size_t _M_size;            // 元素个数（C++11 前 O(n)，C++11 后 O(1)）
};
```

```text
循环链表的哨兵节点（"头节点"，不存数据）：

           ┌──────────────────────────────────────┐
           ↓                                      │
    ┌──────┴──────┐    ┌──────────┐    ┌──────────┐
    │  _M_node    │ ←→ │  node1   │ ←→ │  node2   │
    │ (哨兵)      │    │ data: 10 │    │ data: 20 │
    └─────────────┘    └──────────┘    └──────────┘
```

- `begin()` 返回 `_M_node->_M_next`（第一个实际节点）
- `end()` 返回 `_M_node`（哨兵节点本身）
- 空链表：`_M_node->_M_next == _M_node->_M_prev == _M_node`

### 插入操作的底层（指针操作）

```cpp
/* 源码：libstdc++ bits/list.tcc */

void insert(iterator __position, const value_type& __x) {
    _Node* __tmp = _M_create_node(__x);   // new + 构造
    __tmp->_M_next = __position._M_node;
    __tmp->_M_prev = __position._M_node->_M_prev;
    __position._M_node->_M_prev->_M_next = __tmp;
    __position._M_node->_M_prev = __tmp;
    ++_M_size;
}
```

```text
插入前（在 node1 和 node2 之间插入）：

    node1 ←→ node2
           ↑
        position 指向 node2

插入后：

    node1 ←→ newNode ←→ node2
                      ↑
                   position 仍指向 node2
```

> ✅ **list 插入/删除绝不导致已有迭代器失效**（除了指向被删元素的迭代器）。
> ❌ **不支持随机访问**：`std::advance(it, n)` 需要 O(n)。

### list vs vector 内存对比

```
vector（连续内存）           list（节点分散）
┌──┬──┬──┬──┬──┐        node1 →  [data|next|prev]
│0 │1 │2 │3 │  │        node2 →  [data|next|prev]
└──┴──┴──┴──┴──┘        node3 →  [data|next|prev]

缓存友好：遍历快 ✓         缓存不友好：节点可能散落各处 ✗
随机访问 O(1) ✓           随机访问 O(n) ✗
中间插入 O(n) ✗           中间插入 O(1) ✓
每元素开销：仅数据 ✓       每元素开销：2个指针 + 数据 ✗
```

---

## 3. deque —— 双端队列

### 基本用法

```cpp
#include <deque>

std::deque<int> dq = {1, 2, 3};
dq.push_front(0);          // 头部插入 O(1)
dq.push_back(4);           // 尾部插入 O(1)
dq.pop_front();            // 头部删除 O(1)
dq.pop_back();             // 尾部删除 O(1)
dq[2] = 100;               // 随机访问 O(1)
dq.insert(dq.begin() + 1, 99); // 中间插入 O(n)
```

### 底层实现——分段连续空间（中控器 + 缓冲区）

```
/* 源码：libstdc++ bits/stl_deque.h */

struct _Deque_iterator {
    _Tp* _M_cur;       // 指向当前缓冲区中的当前元素
    _Tp* _M_first;     // 指向当前缓冲区的开头
    _Tp* _M_last;      // 指向当前缓冲区的末尾
    _Map_pointer _M_node; // 指向中控器数组中对应的槽位
};

// deque 本身：
template<typename _Tp, typename _Alloc>
class deque {
    _Map_pointer _M_map;      // 中控器数组（指针数组）
    size_t _M_map_size;       // 中控器数组大小
    _Deque_iterator _M_start; // 指向第一个元素
    _Deque_iterator _M_finish;// 指向最后一个元素的下一个位置
};
```

```text
中控器（map，一块连续内存存指针）：
    ┌────┬────┬────┬────┬────┬────┬────┬────┐
    │    │    │ p1 │ p2 │ p3 │ p4 │    │    │
    └────┴────┴─┬──┴──┬─┴──┬─┴──┬─┴────┴────┘
                ↓     ↓     ↓     ↓
              buf1  buf2  buf3  buf4
            ┌────┐┌────┐┌────┐┌────┐
            │ 10 ││ 20 ││ 30 ││ 40 │     ← 每个 buffer 是连续数组
            │ 11 ││ 21 ││ 31 ││ 41 │        （默认 512 字节，可存
            │    ││ 22 ││    ││    │          int 的话 128 个元素）
            └────┘└────┘└────┘└────┘
               ↑                         ↑
            _M_start                  _M_finish
```

### deque 如何实现"双端 O(1) 插入"？

**头插**：如果当前第一个缓冲区还有前向空间，直接在开头构造；否则在中控器前面新分配一个缓冲区。

```
头插前：                  头插后（push_front(9)）：
    buffer[0]                新 buffer
    ┌────┬────┐              ┌────┐
    │ 0  │ 1  │              │ 9  │ ← 在这个新 buffer 放
    └─┬──┴────┘              └────┘
      ↑                 中控器数组前面多一个指针指向新 buffer
    _M_start
```

### deque 的随机访问（O(1) 但比 vector 慢）

```cpp
/* 源码：libstdc++ bits/stl_deque.h */

reference operator[](size_t __n) {
    return _M_start[_M_offset + __n];
    // 实际上：_M_start + __n 需要跨缓冲区计算
    // 内部实现：先用 __n / __buffer_size 算出在第几个缓冲区
    //          再用 __n % __buffer_size 算出在缓冲区内的偏移
}
```

```text
deque[5] 的访问路径：
1. 计算缓冲区索引：5 / 128 = 0 → 第 0 个 buffer
                   5 % 128 = 5 → 偏移 5
2. 查中控器：*(_M_start._M_node + 0) → buffer0 的地址
3. buffer0[5] → 取值
```

> 比 vector 多了**一次间接寻址**（查中控器），但仍然 O(1)。

### deque 的扩容

- **不重新分配已有元素！** 只在两端新增缓冲区
- 当中控器数组满了才重新分配中控器（拷贝指针，不拷贝数据）
- 中控器扩容也是按 2 倍增长

```
中控器满了（map 两端都被占满）：
    ┌────┬────┬────┬────┐
    │ p1 │ p2 │ p3 │ p4 │
    └────┴────┴────┴────┘
    
扩容中控器（map 变为原来 2 倍，数据移到中间）：
    ┌────┬────┬────┬────┬────┬────┬────┬────┐
    │    │    │ p1 │ p2 │ p3 │ p4 │    │    │
    └────┴────┴────┴────┴────┴────┴────┴────┘
```

> ✅ **deque 头尾插入绝不导致已有迭代器失效**（只有中控器扩容时所有迭代器失效，但几率低）
> ✅ **随机访问 O(1)**（但比 vector 多一次寻址）
> ✅ **比 vector 更节省大块连续内存的压力**（分段连续）

---

## 4. stack / queue —— 容器适配器

### stack（栈）

```cpp
#include <stack>

std::stack<int> stk;             // 默认底层 deque
stk.push(10);                    // 压栈
stk.pop();                       // 弹栈（不返回值！）
int top = stk.top();             // 取栈顶
bool empty = stk.empty();
size_t size = stk.size();
```

### queue（队列）

```cpp
#include <queue>

std::queue<int> q;               // 默认底层 deque
q.push(10);                      // 入队（尾部）
q.pop();                         // 出队（头部，不返回值！）
int front = q.front();           // 队首
int back = q.back();             // 队尾
```

### 底层实现——适配器模式

```
/* 源码：libstdc++ bits/stl_stack.h */

template<typename _Tp, typename _Sequence = deque<_Tp>>
class stack {
protected:
    _Sequence c;  // 底层容器（默认 deque）
public:
    void push(const value_type& __x) { c.push_back(__x); }
    void pop() { c.pop_back(); }         // stack
    // void pop() { c.pop_front(); }     // queue
    
    // stack 和 queue 就只是对底层容器的特定接口封装！
};
```

```text
stack 适配关系：
    push  → push_back
    pop   → pop_back
    top   → back

queue 适配关系：
    push  → push_back
    pop   → pop_front
    front → front
    back  → back
```

**底层容器可替换**：stack 可以用 vector / list，queue 可以用 list

```cpp
std::stack<int, std::vector<int>> stk;  // 用 vector 当底层
std::queue<int, std::list<int>> q;      // 用 list 当底层
```

> 适配器**没有迭代器**，不支持遍历。

---

## 5. priority_queue（优先队列）

### 基本用法

```cpp
#include <queue>

std::priority_queue<int> pq;           // 最大堆（默认 less<int>）
pq.push(30);
pq.push(10);
pq.push(50);
std::cout << pq.top();  // 50（最大值在堆顶）
pq.pop();               // 删除堆顶（50）

// 最小堆
std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
```

### 底层实现——二叉堆（vector + make_heap）

```
/* 源码：libstdc++ bits/stl_queue.h */

template<typename _Tp, typename _Sequence = vector<_Tp>,
         typename _Compare = less<typename _Sequence::value_type>>
class priority_queue {
protected:
    _Sequence c;     // 底层容器（默认 vector）
    _Compare comp;   // 比较谓词
    
public:
    void push(const value_type& __x) {
        c.push_back(__x);               // 先放末尾
        push_heap(c.begin(), c.end(), comp);  // 上滤调整 O(log n)
    }
    
    void pop() {
        pop_heap(c.begin(), c.end(), comp);   // 堆顶和末尾交换 + 下滤
        c.pop_back();                         // 删除末尾（原堆顶）
    }
};
```

```text
二叉堆在 vector 中的存储：
    vector: [ 50 | 30 | 40 | 10 | 20 ]
              0    1    2    3    4
    
    对应的完全二叉树：
              50(0)
             /    \
          30(1)   40(2)
          /   \
       10(3)  20(4)
    
    父子关系公式（0-based）：
        parent(i) = (i-1)/2
        left(i)   = 2*i + 1
        right(i)  = 2*i + 2
```

### push_heap 上滤（percolate up）

```cpp
/* 源码：libstdc++ bits/stl_heap.h */

void push_heap(_RAIter __first, _RAIter __last) {
    _ValueType __value = *(__last - 1);  // 新插入的值
    _Distance __hole = __last - __first - 1;  // 新元素位置（空洞）
    
    // 上滤：从 __hole 开始，不断与父节点比较
    while (__hole > 0) {
        _Distance __parent = (__hole - 1) / 2;
        if (!(__first[__parent] < __value)) break;  // 父大则停
        __first[__hole] = __first[__parent];         // 父节点下移
        __hole = __parent;                           // 空洞上移
    }
    __first[__hole] = __value;  // 放入最终位置
}
```

```text
插入 60 到 [50, 30, 40, 10, 20]：

    [50, 30, 40, 10, 20, 60]
                           ↑
                        空洞(hole=5)
    
    父节点 (5-1)/2=2 → 40 < 60 → 40 下移
    [50, 30, 40, 10, 20, 40]  hole = 2
    
    父节点 (2-1)/2=0 → 50 < 60 → 50 下移
    [50, 30, 50, 10, 20, 40]  hole = 0
    
    到达堆顶 → 放入 60 → [60, 30, 50, 10, 20, 40]
```

---

# 四、关联式容器（基于红黑树）

## 红黑树基础（RB-Tree）

map / set 的底层是**红黑树**——一种自平衡二叉查找树。

### 红黑树规则

```text
1. 每个节点是红色或黑色
2. 根节点是黑色
3. 叶子节点（NIL）是黑色
4. 红色节点的子节点必须是黑色（不能有连续红节点）
5. 从任一节点到其叶子的所有路径包含相同数量的黑色节点（黑高平衡）
```

### GCC 红黑树源码结构

```cpp
/* 源码：libstdc++ bits/stl_tree.h */

struct _Rb_tree_node_base {
    _Rb_tree_color    _M_color;   // _S_red 或 _S_black
    _Rb_tree_node_base* _M_parent;
    _Rb_tree_node_base* _M_left;
    _Rb_tree_node_base* _M_right;
};

template<typename _Val>
struct _Rb_tree_node : public _Rb_tree_node_base {
    _Val _M_storage;  // 实际数据（通过 __gnu_cxx::_Hashtable_alloc 管理）
};

template<typename _Key, typename _Val, typename _KeyOfValue,
         typename _Compare, typename _Alloc>
class _Rb_tree {
protected:
    size_t _M_node_count;
    _Rb_tree_node_base* _M_header; // 哨兵节点，结构类似 list 的哨兵
public:
    // 核心操作
    pair<iterator, bool> _M_insert_unique(const _Val& __v);
    iterator _M_insert_equal(const _Val& __v);
    void _M_erase(iterator __position);
};
```

```text
_Rb_tree 内部结构（哨兵模式）：

    _M_header（哨兵）
    ├── _M_parent → 根节点
    ├── _M_left   → 最左节点（最小元素）
    └── _M_right  → 最右节点（最大元素）

        黑(50)
       /       \
    红(30)    红(70)
    /   \      /   \
 黑(20)黑(40)黑(60)黑(80)
```

---

## 2. set / multiset

### 基本用法

```cpp
#include <set>

std::set<int> s;                 // 有序集合（默认 less<int>）
s.insert(30);
s.insert(10);
s.insert(50);
s.insert(20);
// s 内部: {10, 20, 30, 50}  自动升序排列

auto it = s.find(30);            // 查找 O(log n)
if (it != s.end()) s.erase(it);  // 删除

s.count(20);                     // 返回 1（存在）或 0（不存在）
s.lower_bound(20);               // 第一个 ≥ 20 的迭代器
s.upper_bound(30);               // 第一个 > 30 的迭代器

// multiset：允许重复
std::multiset<int> ms;
ms.insert(10);
ms.insert(10);                   // 允许重复
ms.count(10);                    // 返回 2
```

### 底层实现

```
/* 源码：libstdc++ bits/stl_set.h */

template<typename _Key, typename _Compare = less<_Key>,
         typename _Alloc = allocator<_Key>>
class set {
public:
    typedef _Key     key_type;
    typedef _Key     value_type;  // set 的 key 和 value 是同一个

private:
    typedef _Rb_tree<key_type, value_type, 
                     _Identity<value_type>, _Compare, _Alloc> _Rep_type;
    _Rep_type _M_t;  // 底层红黑树
};
```

set 本质上就是 `_Rb_tree` 的一层薄封装：

```cpp
// 核心接口直接转发给红黑树
std::pair<iterator, bool> insert(const value_type& __x) {
    return _M_t._M_insert_unique(__x);  // 唯一插入（set 不允许重复）
}

iterator begin() { return _M_t.begin(); }
iterator end()   { return _M_t.end(); }
```

> ⚠️ **set 的迭代器是 const 的**：不能通过迭代器修改元素值，这会破坏红黑树的有序性。

---

## 3. map / multimap

### 基本用法

```cpp
#include <map>

std::map<std::string, int> ages;
ages["Alice"] = 25;           // 插入/访问 O(log n)
ages["Bob"]   = 30;
ages["Charlie"] = 35;

// 查找
auto it = ages.find("Bob");
if (it != ages.end())
    std::cout << it->first << ": " << it->second << std::endl;

// 遍历（按键升序）
for (const auto& [name, age] : ages)
    std::cout << name << ": " << age << std::endl;

// 检查是否存在
if (ages.count("Alice"))  // 返回 0 或 1（multimap 可返回 >1）

// multimap：允许多个相同 key
std::multimap<std::string, int> mm;
mm.insert({"key", 1});
mm.insert({"key", 2});  // 同一 key 可以对应多个 value
auto range = mm.equal_range("key");  // 返回 [begin, end) 覆盖所有 "key"
```

### 底层实现

```
/* 源码：libstdc++ bits/stl_map.h */

template<typename _Key, typename _Tp, typename _Compare = less<_Key>,
         typename _Alloc = allocator<pair<const _Key, _Tp>>>
class map {
public:
    typedef _Key                  key_type;
    typedef _Tp                   mapped_type;
    typedef pair<const _Key, _Tp> value_type;  // 注意：key 部分是 const！

private:
    typedef _Rb_tree<key_type, value_type, 
                     _Select1st<value_type>, _Compare, _Alloc> _Rep_type;
    _Rep_type _M_t;  // 底层红黑树
};
```

### operator[] 的底层逻辑

```cpp
/* 源码：libstdc++ bits/stl_map.h */

mapped_type& operator[](const key_type& __k) {
    iterator __i = lower_bound(__k);   // 查找
    if (__i == end() || key_comp()(__k, (*__i).first))
        __i = _M_t._M_emplace_hint_unique(__i, std::piecewise_construct,
                                          std::tuple<const key_type&>(__k),
                                          std::tuple<>());
    // ↑ 没找到就插入一个默认构造的 value
    return (*__i).second;
}
```

```text
map["Alice"] = 25 等价于两步：
1. 查找 "Alice"：
   - 找到了 → 返回 value 引用，直接赋值 25
   - 没找到 → 插入 pair("Alice", 0) 然后返回 value 引用，赋值 25
```

> ⚠️ **operator[] 会在 key 不存在时自动插入默认值！**
> 如果你只是想检查存在性，用 `find()` 或 `count()`，别用 `operator[]`。

### 红黑树插入与旋转（源码视角）

```cpp
/* 源码：libstdc++ bits/stl_tree.h */

void _Rb_tree_insert_and_rebalance(const bool __insert_left,
    _Rb_tree_node_base* __x, _Rb_tree_node_base* __p,
    _Rb_tree_node_base& __header) 
{
    _Rb_tree_node_base*& __root = __header._M_parent;
    
    // 插入节点（先按 BST 规则放入）
    __x->_M_parent = __p;
    __x->_M_left = nullptr;
    __x->_M_right = nullptr;
    __x->_M_color = _S_red;  // 新节点总为红色
    
    // 调整平衡（核心）
    while (__x != __root && __x->_M_parent->_M_color == _S_red) {
        // 父节点是祖父节点的左子还是右子
        if (__x->_M_parent == __x->_M_parent->_M_parent->_M_left) {
            // 情况 1: 叔叔是红色 → 颜色翻转
            // 情况 2: 叔叔是黑色且 x 是右子 → 左旋
            // 情况 3: 叔叔是黑色且 x 是左子 → 右旋
        } else {
            // 对称处理
        }
    }
    __root->_M_color = _S_black;  // 根始终为黑
}
```

**红黑树插入的三种调整情况**：

```text
插入新节点 35（红）到树中，破坏规则4（红节点不能有红子节点）：

情况1：叔叔是红色 → 颜色翻转
        黑(30)               黑(30)
       /    \              /    \
    红(20)  红(40)  →    黑(20)  黑(40)    ← 颜色翻转
            /                      /
         红(35)                 红(35)

情况2/3：叔叔是黑色 → 旋转 + 重新着色
    （略，比 AVL 树旋转次数少得多）
```

> 红黑树保证了 **O(log n) 的插入/删除/查找**，且每次插入最多 2 次旋转，删除最多 3 次旋转。

---

# 五、哈希容器（unordered_*）

## unordered_map / unordered_set

C++11 引入的哈希容器，与 map/set 的区别：

| | map/set | unordered_map/unordered_set |
|---|---|---|
| 底层 | 红黑树 | 哈希表（桶 + 链表） |
| 元素顺序 | 有序（按 key 排序） | 无序 |
| 操作复杂度 | O(log n) | O(1) 平均，O(n) 最坏 |
| 需要头文件 | `<map>` / `<set>` | `<unordered_map>` / `<unordered_set>` |

### 基本用法

```cpp
#include <unordered_map>
#include <unordered_set>

std::unordered_map<std::string, int> umap;
umap["apple"] = 5;
umap["banana"] = 3;

std::unordered_set<int> uset = {3, 1, 4, 1, 5};
// 遍历顺序不确定，可能是 {5, 3, 1, 4} 之类的顺序
```

### 底层实现——桶 + 链表（开链法）

```
/* 源码：libstdc++ bits/hashtable.h */

// 哈希表结构（核心概念）
// _M_buckets: 桶数组（vector 存储节点指针）
// 每个桶指向一个单向链表的头节点

template<typename _Key, typename _Value,
         typename _Alloc, typename _ExtractKey,
         typename _Equal, typename _H1, typename _H2,
         typename _Hash, typename _RehashPolicy,
         typename _Traits>
class _Hashtable {
    _Bucket*        _M_buckets;      // 桶数组（动态数组）
    size_t          _M_bucket_count; // 桶数量（通常为质数）
    _Node_base      _M_before_begin; // 哨兵节点（辅助遍历）
    size_t          _M_element_count;// 元素总数
};
```

```text
哈希表结构：

        buckets[]         链表
        ┌──────┐
    [0] │  ◆───┼──→ [ 20 | next ] → [ 30 | next ] → null
        ├──────┤
    [1] │ null │
        ├──────┤
    [2] │  ◆───┼──→ [ 55 | next ] → null
        ├──────┤
    [3] │  ◆───┼──→ [ 12 | next ] → [ 45 | next ] → null
        ├──────┤
    [4] │ null │
        └──────┘

插入 key=5：
    hash(5) % bucket_count → 假设得到 0
    遍历桶 0 的链表，没找到 key=5 → 头插新节点
```

### 哈希冲突与扩容

```cpp
/* 源码：libstdc++ bits/hashtable.h */

// 插入触发 rehash 的条件
if (_M_element_count > _M_bucket_count * _M_max_load_factor)
    _M_rehash(_M_bucket_count + 1);  // 找下一个质数作为新桶数
```

```text
负载因子 load_factor = 元素总数 / 桶数
默认 max_load_factor = 1.0

当元素数超过桶数 × max_load_factor 时：
1. 新桶数 = 大于当前桶数 × 2 的下一个质数
            （如 13 → 29 → 53 → 97...）
2. 创建新桶数组
3. 遍历旧桶所有元素，重新计算 hash 放入新桶
4. 释放旧桶 → 这叫 rehash，O(n)

为什么桶数取质数？
因为 hash % 桶数 能更均匀地分布，减少聚集
```

### 自定义哈希函数

```cpp
// 自定义类型需要提供 hash 和 equal
struct Person {
    std::string name;
    int age;
    bool operator==(const Person& other) const {
        return name == other.name && age == other.age;
    }
};

// 特化 std::hash
namespace std {
    template<>
    struct hash<Person> {
        size_t operator()(const Person& p) const {
            return hash<string>()(p.name) ^ hash<int>()(p.age);
        }
    };
}

std::unordered_set<Person> people;
```

---

# 六、迭代器深入

> 迭代器更详细的用法见 `base_iterator.md`，这里补充迭代器与底层容器的关系。

## 迭代器分类（按能力层级）

```
                     Input           ← 只读，单遍扫描
                    /    \
             Forward        Output   ← 只写，单遍扫描
                |
             Bidirectional           ← 双向移动（list, set, map）
                |
           Random Access             ← 随机访问（vector, deque, array, string）
                |
         Contiguous Iterator (C++17) ← 连续内存（vector, string, array）
```

### 各类容器对应的迭代器类型

| 容器 | 迭代器类型 | 能力 |
|------|-----------|------|
| `vector` | Random Access | `++`,`--`,`+`,`-`,`[]`,`<`,`>` |
| `deque` | Random Access | 同上（但底层是分段连续） |
| `array` | Random Access | 同上 |
| `string` | Random Access | 同上 |
| `list` | Bidirectional | `++`,`--`，无 `+ n` |
| `set`/`map` | Bidirectional | 同上 |
| `unordered_set`/`unordered_map` | Forward | 仅 `++`，无 `--` |
| `stack`/`queue` | **无迭代器** | 适配器不暴露迭代器 |

### 迭代器失效规则

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
auto it = v.begin() + 2;  // 指向 3

v.push_back(6);  // 可能扩容 → it 失效！
// *it = 99;     // ❌ 未定义行为！

v.insert(v.begin(), 0);  // 全部元素后移 → it 失效
v.erase(v.begin());      // 删除 it 及之后的都失效
v.erase(v.begin() + 2);  // 只有被删元素之后的迭代器失效
```

| 操作 | vector | deque | list | set/map | unordered_* |
|------|--------|-------|------|---------|-------------|
| 插入 | 全部失效（扩容）或插入点后 | 头尾插入：不失效；中间：全部失效 | 不失效 | 不失效 | 不失效(rehash时全部失效) |
| 删除 | 删除点后全部失效 | 删除点后全部失效 | 仅被删元素失效 | 仅被删元素失效 | 仅被删元素失效 |
| resize/assign | 全部失效 | 全部失效 | 不失效 | — | — |

---

# 七、算法（Algorithms）与容器的协作

STL 算法通过迭代器操作容器，不直接依赖容器类型。

```cpp
#include <algorithm>

std::vector<int> v = {5, 2, 8, 1, 9};
std::sort(v.begin(), v.end());                 // 快速排序（RandomAccess 迭代器）
auto it = std::find(v.begin(), v.end(), 8);     // 线性查找
std::reverse(v.begin(), v.end());               // 反转
std::unique(v.begin(), v.end());                // 去重（相邻重复）
std::lower_bound(v.begin(), v.end(), 5);        // 二分查找下界
std::for_each(v.begin(), v.end(), [](int x){
    std::cout << x << " ";
});
```

### 为什么 list 不能用 std::sort？

```cpp
std::list<int> lst = {5, 2, 8, 1, 9};
// std::sort(lst.begin(), lst.end());  // ❌ 编译错误！
// std::sort 需要 RandomAccessIterator（支持 it + n）
// list 只有 BidirectionalIterator（只支持 ++/--）

lst.sort();  // ✅ list 有自己的成员 sort（归并排序）
```

### 迭代器适配器

```cpp
// 反向迭代器
for (auto rit = v.rbegin(); rit != v.rend(); ++rit)
    std::cout << *rit << " ";  // 反向遍历

// 插入迭代器
std::vector<int> src = {1, 2, 3}, dst;
std::copy(src.begin(), src.end(), std::back_inserter(dst));
// back_inserter 每次赋值时调用 push_back
```

---

# 八、总结——各容器选择指南

| 需求 | 选哪个 | 理由 |
|------|--------|------|
| 连续内存、随机访问快 | `vector` | O(1) 随机访问，缓存友好 |
| 频繁头尾插入删除 | `deque` | 头尾 O(1)，且支持随机访问 |
| 频繁中间插入删除 | `list` | 中间插入 O(1)，迭代器不失效 |
| FIFO 队列 | `queue` | 适配 deque，接口简洁 |
| LIFO 栈 | `stack` | 适配 deque/vector |
| 优先队列（最大/最小） | `priority_queue` | 堆实现，O(log n) 插入/弹出 |
| 有序唯一 key 映射 | `map` | 红黑树，O(log n) |
| 有序唯一元素集合 | `set` | 红黑树，O(log n) |
| 无需排序，快速查找 | `unordered_map` | 哈希表，平均 O(1) |
| 无需排序，快速去重 | `unordered_set` | 哈希表，平均 O(1) |

### 内存开销对比

| 容器 | 每元素额外开销 | 内存连续性 |
|------|--------------|-----------|
| `vector` | 0（仅数据） | 连续 |
| `deque` | ~8 字节（中控器指针分摊） | 分段连续 |
| `list` | 16 字节（64 位下 prev + next 指针） | 离散 |
| `set`/`map` | 24~40 字节（红黑树父/左/右/颜色 + 对齐） | 离散 |
| `unordered_*` | 8~16 字节（next 指针 + 哈希值开销） | 离散 |

---

> 参考：libstdc++ (GCC) 源码 `bits/stl_vector.h`、`bits/stl_list.h`、`bits/stl_deque.h`、`bits/stl_tree.h`、`bits/stl_queue.h`、`bits/hashtable.h`

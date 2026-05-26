#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>  // mmap

// <cstddef>中定义max_align_t也就是最大对齐要求，x86-64是16B
static constexpr size_t ALIGNMENT = alignof(std::max_align_t);

// align - 1生成掩码MASK
// ~15 == 1111 0000任何数与之相与其低四位都是0
#define ALIGN_UP(size, align) (((size) + (align) - 1) & ~((align) - 1))

// 一页大部分为4KB（这里是向OS申请内存的最小粒度）
static constexpr size_t MMAP_THRESHOLED = 4096;

/**
 *  Metadata Header
 *  每个分配的内存块都含该结构体
 */

struct BlockHeader
{
    size_t size;
    bool is_free;
    BlockHeader* next;
};

// 保证Header本身对齐，这样后面的payload才能安全对齐
static constexpr size_t HEADER_SIZE = ALIGN_UP(sizeof(BlockHeader), ALIGNMENT);

// 空闲链表头，采用单链表管理这里
static BlockHeader* g_heap_head = nullptr;


// === OS ===

static void* request_os_mem(size_t size)
{
    // PROT_READ | PROT_WRITE: 可读可写
    // MAP_PRIVATE | MAP_ANONYMOUS: 私有匿名映射，不关联文件，初始化为0
    void* p = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    return (p == MAP_FAILED) ? nullptr : p;
}

static void release_os_mem(void* ptr, size_t size)
{
    munmap(ptr, size);
}

// 初始化新堆区
static BlockHeader* init_new_region(size_t payload_size)
{
    size_t total_size = HEADER_SIZE + payload_size;
    total_size = ALIGN_UP(total_size, MMAP_THRESHOLED);

    BlockHeader* block = (BlockHeader*)request_os_mem(total_size);
    if (!block) return nullptr;

    block->size = total_size - HEADER_SIZE;
    block->is_free = true;
    block->next = nullptr;

    if (!g_heap_head) g_heap_head = block;
    else
    {
        BlockHeader* curr = g_heap_head;
        // 尾插法
        while (curr->next) curr = curr->next;
        curr->next = block;
    }

    return block;
}

// 在现有链表中找合适空闲块
static BlockHeader* find_free_block(size_t size)
{
    BlockHeader* curr = g_heap_head;
    while (curr)
    {
        if (curr->is_free && curr->size >= size) return curr;
        curr = curr->next;
    }
    return nullptr;
}

static void split_block(BlockHeader* block, size_t needed_payload)
{
    size_t remaining = block->size - needed_payload;
    if (remaining > HEADER_SIZE + ALIGNMENT)
    {
        BlockHeader* new_block = (BlockHeader*)((uint8_t*)block + HEADER_SIZE + needed_payload);
        new_block->size = remaining - HEADER_SIZE;
        new_block->is_free = true;
        new_block->next = block->next;

        block->size = needed_payload;
        block->next = new_block;
    }
}

// 合并相邻的空闲块 (消除外部碎片)
static void coalesce() {
    BlockHeader* curr = g_heap_head;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            // 将下一个块吞并
            curr->size += HEADER_SIZE + curr->next->size;
            curr->next = curr->next->next;
            // 注意：不要移动 curr，因为合并后可能还能继续和后面的块合并
        } else {
            curr = curr->next;
        }
    }
}


void* my_malloc(size_t size) {
    if (size == 0) return nullptr;
    size = ALIGN_UP(size, ALIGNMENT);

    // 1. 尝试从已有链表中找
    BlockHeader* block = find_free_block(size);
    
    // 2. 找不到则向 OS 申请新区域
    if (!block) {
        block = init_new_region(size);
        if (!block) return nullptr;
    }

    // 3. 标记占用并尝试分割
    block->is_free = false;
    split_block(block, size);

    // 返回跳过 Header 的 payload 地址
    return (void*)((uint8_t*)block + HEADER_SIZE);
}

void my_free(void* ptr) {
    if (!ptr) return;

    // 通过 payload 指针反推 Header 地址
    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - HEADER_SIZE);
    block->is_free = true;

    // 释放后立即尝试合并相邻空闲块
    coalesce();

    // 💡 进阶优化点：如果整个 region 都是 free 且达到一定大小，
    // 可以调用 release_os_memory 归还给系统。此处省略以保持简洁。
}

void* my_realloc(void* ptr, size_t new_size) {
    if (new_size == 0) { my_free(ptr); return nullptr; }
    if (!ptr) return my_malloc(new_size);

    new_size = ALIGN_UP(new_size, ALIGNMENT);
    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - HEADER_SIZE);

    // ✅ 完美解决之前提到的"不知道旧大小"问题！
    // 直接从 metadata 读取真实的 old_size
    size_t old_size = block->size;

    // 场景A: 缩小或大小不变，直接返回原指针 (可选: 触发 split_block)
    if (old_size >= new_size) {
        split_block(block, new_size); // 把多出来的部分切出去变成空闲块
        return ptr;
    }

    // 场景B: 扩大，且下一个物理相邻块恰好是空闲的且够大 -> 原地扩展！
    if (block->next && block->next->is_free) {
        size_t combined = old_size + HEADER_SIZE + block->next->size;
        if (combined >= new_size) {
            // 吞并下一个块
            block->size = new_size;
            size_t leftover = combined - new_size;
            
            if (leftover > HEADER_SIZE + ALIGNMENT) {
                // 重新设置被吞并块的剩余部分
                BlockHeader* remainder = (BlockHeader*)((uint8_t*)block + HEADER_SIZE + new_size);
                remainder->size = leftover - HEADER_SIZE;
                remainder->is_free = true;
                remainder->next = block->next->next;
                block->next = remainder;
            } else {
                // 剩余太少，全部吃掉
                block->next = block->next->next;
            }
            return ptr; // 零拷贝原地扩容！
        }
    }

    // 场景C: 无法原地扩展，只能 分配+拷贝+释放
    void* new_ptr = my_malloc(new_size);
    if (!new_ptr) return nullptr; // 失败时原 ptr 依然有效，符合 realloc 语义

    std::memcpy(new_ptr, ptr, old_size); // ✅ 精确拷贝 old_size，绝不越界
    my_free(ptr);
    return new_ptr;
}

/* 
void* my_realloc(void* ptr, size_t new_size)
{
    if (new_size == 0)
    {
        free(ptr);
        return nullptr;
    }

    if (ptr == nullptr)
        return malloc(new_size);

    void* new_ptr = malloc(new_size);
    if (new_ptr == nullptr) return nullptr;

    memcpy(new_ptr, ptr, new_size);

    free(ptr);
    return new_ptr;
}
*/

int main()
{
    // 测试1: 基本扩容
    int* arr = (int*)my_realloc(nullptr, 4 * sizeof(int));
    for (int i = 0; i < 4; i++) arr[i] = i * 10;

    arr = (int*)my_realloc(arr, 8 * sizeof(int));
    for (int i = 0; i < 8; i++)
        std::printf("arr[%d] = %d\n", i, arr[i]); // 前4个应为 0,10,20,30

    // 测试2: 缩容
    arr = (int*)my_realloc(arr, 2 * sizeof(int));
    for (int i = 0; i < 2; i++)
        std::printf("shrunk[%d] = %d\n", i, arr[i]); // 应为 0,10

    my_realloc(arr, 0); // 等价于 free
    return 0;
}
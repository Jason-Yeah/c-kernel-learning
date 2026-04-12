#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* --- 第一部分：底层的堆模拟系统 (Simplified memlib.c) --- */

#define MAX_HEAP (20 * (1<<20))  /* 20 MB 堆空间 */
static char *mem_heap;           /* 堆的起始地址 */
static char *mem_brk;            /* 当前堆的结束地址 (brk) */
static char *mem_max_addr;       /* 堆的最大允许地址 */

void mem_init() {
    mem_heap = (char *)malloc(MAX_HEAP);
    mem_brk = (char *)mem_heap;
    mem_max_addr = (char *)(mem_heap + MAX_HEAP);
}

void *mem_sbrk(int incr) {
    char *old_brk = mem_brk;
    if ((incr < 0) || ((mem_brk + incr) > mem_max_addr)) return (void *)-1;
    mem_brk += incr;
    return (void *)old_brk;
}

/* --- 第二部分：核心宏定义 --- */

#define WSIZE       4       
#define DSIZE       8       
#define CHUNKSIZE  (1<<12)  /* 4KB */

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp = 0;

/* --- 第三部分：核心函数实现 --- */

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) return bp;

    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

int mm_init() {
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) return -1;
    PUT(heap_listp, 0); 
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* 序言块 Header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* 序言块 Footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* 结尾块 Header */
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) return -1;
    return 0;
}

static void *find_fit(size_t asize) {
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

void *mm_malloc(size_t size) {
    size_t asize, extendsize;
    char *bp;
    if (size == 0) return NULL;
    if (size <= DSIZE) asize = 2*DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/* --- 第四部分：测试代码 --- */

int main() {
    mem_init();
    if (mm_init() < 0) {
        printf("Init failed\n");
        return -1;
    }

    printf("--- 1. 测试内存复用 ---\n");
    void *p1 = mm_malloc(128);
    printf("P1 malloc at: %p\n", p1);
    
    mm_free(p1);
    printf("P1 freed.\n");

    void *p3 = mm_malloc(128); 
    printf("P3 malloc at: %p (Should be same as P1)\n", p3);

    printf("\n--- 2. 测试碎片合并 (Coalesce) ---\n");
    void *p4 = mm_malloc(64);
    void *p5 = mm_malloc(64);
    printf("P4 at: %p, P5 at: %p\n", p4, p5);

    mm_free(p4);
    mm_free(p5);
    printf("P4 and P5 freed. Now they should be merged into one 128+ block.\n");

    void *p6 = mm_malloc(128);
    printf("P6 malloc at: %p (Should reuse P4's start address)\n", p6);

    return 0;
}

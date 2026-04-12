#include <stdio.h>
#include <stdlib.h>

#define WSIZE       4       
#define DSIZE       8       
#define CHUNKSIZE  (1<<12) 
#define SEGLIST_COUNT 15    
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc))
#define GET(p)             (*(unsigned int *)(p))
#define PUT(p, val)        (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)        (GET(p) & ~0x7)
#define GET_ALLOC(p)       (GET(p) & 0x1)

#define HDRP(bp)           ((char *)(bp) - WSIZE)
#define FTRP(bp)           ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)      ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)      ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define GET_PRED(bp)       (*(char **)(bp))
#define GET_SUCC(bp)       (*(char **)((char *)(bp) + WSIZE))
#define SET_PRED(bp, val)  (*(char **)(bp) = (char *)(val))
#define SET_SUCC(bp, val)  (*(char **)((char *)(bp) + WSIZE) = (char *)(val))

static char *free_lists[SEGLIST_COUNT]; 
static char *mem_heap; 
static char *mem_brk;
static char *heap_listp = 0;

void mem_init() {
    mem_heap = (char *)malloc(20 * 1024 * 1024);
    mem_brk = mem_heap;
}

void *mem_sbrk(int incr) {
    char *old_brk = mem_brk;
    if (incr < 0) return (void *)-1;
    mem_brk += incr;
    return (void *)old_brk;
}

int get_index(size_t size) {
    for (int i = 0; i < SEGLIST_COUNT - 1; i++) {
        if (size <= (32 << i)) return i;
    }
    return SEGLIST_COUNT - 1;
}

void insert_node(void *bp, size_t size) {
    int index = get_index(size);
    /* 核心调试信息 */
    printf("[DEBUG] Inserting block of size %zu into free_lists[%d]\n", size, index);

    SET_SUCC(bp, free_lists[index]);
    SET_PRED(bp, NULL);
    if (free_lists[index] != NULL) SET_PRED(free_lists[index], bp);
    free_lists[index] = (char *)bp;
}

static void *find_fit(size_t asize) {
    int index = get_index(asize);
    /* 核心调试信息 */
    printf("[DEBUG] Searching for size %zu starting from free_lists[%d]...\n", asize, index);

    for (int i = index; i < SEGLIST_COUNT; i++) {
        for (char *bp = free_lists[i]; bp != NULL; bp = GET_SUCC(bp)) {
            if (asize <= GET_SIZE(HDRP(bp))) {
                printf("[DEBUG] Found match in list[%d]!\n", i);
                return bp;
            }
        }
    }
    return NULL;
}

void delete_node(void *bp) {
    int index = get_index(GET_SIZE(HDRP(bp)));
    if (GET_PRED(bp) != NULL) SET_SUCC(GET_PRED(bp), GET_SUCC(bp));
    else free_lists[index] = GET_SUCC(bp);
    if (GET_SUCC(bp) != NULL) SET_PRED(GET_SUCC(bp), GET_PRED(bp));
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (!prev_alloc) {
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    if (!next_alloc) {
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    insert_node(bp, size);
    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 新的结尾块
    return coalesce(bp);
}

int mm_init() {
    for (int i = 0; i < SEGLIST_COUNT; i++) free_lists[i] = NULL;
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1;
    PUT(heap_listp, 0);                          
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 序言块 Header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 序言块 Footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 结尾块 Header
    heap_listp += (2 * WSIZE);
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
    return 0;
}

static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    delete_node(bp);
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert_node(bp, csize - asize);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

void *mm_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t asize = (size <= DSIZE) ? 2 * DSIZE : DSIZE * ((size + DSIZE + 7) / DSIZE);
    for (int i = get_index(asize); i < SEGLIST_COUNT; i++) {
        for (char *bp = free_lists[i]; bp != NULL; bp = GET_SUCC(bp)) {
            if (asize <= GET_SIZE(HDRP(bp))) {
                place(bp, asize);
                return bp;
            }
        }
    }
    size_t extendsize = MAX(asize, CHUNKSIZE);
    char *bp = extend_heap(extendsize / WSIZE);
    if (bp == NULL) return NULL;
    place(bp, asize);
    return bp;
}

void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

int main() {
    mem_init();
    if (mm_init() < 0) return -1;
    printf("--- 分离适配测试 ---\n");
    void *p1 = mm_malloc(128);
    printf("P1 allocated at %p\n", p1);
    mm_free(p1);
    void *p2 = mm_malloc(128);
    printf("P2 allocated at %p\n", p2);
    if (p1 == p2) printf("SUCCESS: Reused P1!\n");
    return 0;
}

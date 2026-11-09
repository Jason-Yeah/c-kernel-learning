#define _GNU_SOURCE
/* VA and values are observable; physical PFNs/COW copies are not inferred here. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

volatile int trace_phase, role;
pid_t fork_result, child_id;
volatile int *private_page, *shared_page;
long page_size;
volatile int global_data = 10;
volatile int global_bss;
volatile int *stack_local;
volatile int *heap_object;

__attribute__((noinline)) static void checkpoint(int phase)
{
    trace_phase = phase;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_FORK_MEMORY_EVENT */
}
static int transfer(int fd, int writing)
{
    char byte = 'x';
    return (writing ? write(fd, &byte, 1) : read(fd, &byte, 1)) == 1;
}
int main(void)
{
    page_size = sysconf(_SC_PAGESIZE);
    private_page = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    shared_page = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (private_page == MAP_FAILED || shared_page == MAP_FAILED)
        return 2;
    *private_page = 40;
    *shared_page = 50;
    volatile int local = 20;
    stack_local = &local;
    heap_object = malloc(sizeof *heap_object);
    if (heap_object == NULL)
        return 10;
    *heap_object = 30;
    int go[2], done[2];
    if (pipe(go) != 0 || pipe(done) != 0)
        return 3;
    fork_result = fork();
    if (fork_result < 0)
        return 4;
    if (fork_result == 0) {
        role = 1;
        child_id = getpid();
        close(go[1]);
        close(done[0]);
        if (!transfer(go[0], 0))
            _exit(5);
        checkpoint(2); /* child before mutation: same VA, original values */
        global_data = 100;
        global_bss = 200;
        *stack_local = 200;
        *heap_object = 300;
        *private_page = 400;
        *shared_page = 500;
        checkpoint(3); /* child's actual values */
        if (!transfer(done[1], 1))
            _exit(6);
        _exit(0);
    }
    child_id = fork_result;
    close(go[0]);
    close(done[1]);
    checkpoint(1); /* parent before releasing child */
    if (!transfer(go[1], 1) || !transfer(done[0], 0))
        return 7;
    int status;
    if (waitpid(child_id, &status, 0) != child_id || status != 0)
        return 8;
    checkpoint(4); /* parent: private still 40, shared now 500 */
    printf("parent private=%d shared=%d\n", *private_page, *shared_page);
    return *private_page == 40 && *shared_page == 500 ? 0 : 9;
}

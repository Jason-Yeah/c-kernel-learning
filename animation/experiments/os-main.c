#define _GNU_SOURCE
#include "os-demo.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

volatile int os_phase;
volatile int os_role;
volatile int os_result;
volatile int os_equal_result;
volatile uint64_t os_width_result;
volatile int os_initial_data;
volatile int *os_stack;
int *os_heap;
int *os_private;
int *os_shared;
long os_page_size;
pid_t os_fork_result = -1;
pid_t os_child = -1;

__attribute__((noinline)) void os_checkpoint(int phase)
{
    os_phase = phase;
    __asm__ volatile("nop" ::: "memory"); /* TRACE_OS_EVENT */
}

static void transfer(int fd, int writing)
{
    char byte = 'X';
    ssize_t count;
    do {
        count = writing ? write(fd, &byte, 1) : read(fd, &byte, 1);
    } while (count < 0 && errno == EINTR);
    if (count != 1)
        _exit(20);
}

int main(int argc, char **argv)
{
    struct rlimit core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &core) != 0)
        return 1;
    volatile int local = 20;
    os_stack = &local;
    os_page_size = sysconf(_SC_PAGESIZE);
    if (os_page_size <= 0)
        return 2;
    os_heap = malloc(sizeof *os_heap);
    os_private = mmap(NULL, (size_t)os_page_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    os_shared = mmap(NULL, (size_t)os_page_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!os_heap || os_private == MAP_FAILED || os_shared == MAP_FAILED)
        return 3;
    *os_heap = 30;
    *os_private = 40;
    *os_shared = 50;
    os_initial_data = os_data;
    os_checkpoint(1);

    os_result = os_add(OS_LEFT, OS_RIGHT);
    os_width_result = os_width();
    os_equal_result = os_equal(OS_LEFT, OS_LEFT);
    os_checkpoint(2);

    if (argc == 2 && strcmp(argv[1], "fork") == 0) {
        int done[2], proceed[2];
        if (pipe(done) || pipe(proceed))
            return 4;
        os_fork_result = fork();
        if (os_fork_result < 0)
            return 5;
        if (os_fork_result == 0) {
            os_role = 1;
            os_child = getpid();
            close(done[0]);
            close(proceed[1]);
            os_checkpoint(3);
            os_data = 100;
            os_bss = 200;
            local = 200;
            *os_heap = 300;
            *os_private = 400;
            *os_shared = 500;
            os_checkpoint(4);
            transfer(done[1], 1);
            transfer(proceed[0], 0);
            _exit(0);
        }
        os_child = os_fork_result;
        close(done[1]);
        close(proceed[0]);
        transfer(done[0], 0);
        os_checkpoint(5);
        transfer(proceed[1], 1);
        int status;
        if (waitpid(os_child, &status, 0) != os_child || status != 0)
            return 6;
        close(done[0]);
        close(proceed[1]);
    }
    printf("%s PID=%ld sum=%d data=%d bss=%d local=%d heap=%d private=%d shared=%d\n",
           os_message, (long)getpid(), os_result, os_data, os_bss, local,
           *os_heap, *os_private, *os_shared);
    free(os_heap);
    if (munmap(os_private, (size_t)os_page_size) ||
        munmap(os_shared, (size_t)os_page_size))
        return 7;
    return os_result == 5 && os_width_result == 5 && os_equal_result == 1 ? 0 : 8;
}

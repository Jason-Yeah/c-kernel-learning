/* A process-local stack limit makes exhaustion reproducible without host changes. */
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

volatile unsigned bounded_depth;
volatile unsigned observed_sum;
volatile int trace_phase;
unsigned long stack_limit;

__attribute__((noinline)) static unsigned f(unsigned depth)
{
    volatile unsigned char block[4096];
    block[0] = (unsigned char)depth;
    block[sizeof block - 1] = (unsigned char)depth;
    trace_phase = 1;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_RECURSION_ENTER */
    unsigned child = 0;
    if (bounded_depth == 0 || depth < bounded_depth)
        child = f(depth + 1);
    trace_phase = 2;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_RECURSION_LEAVE */
    return child + block[0];
}

int main(int argc, char **argv)
{
    struct rlimit limit, no_core = {0, 0};
    if (argc != 2 || getrlimit(RLIMIT_STACK, &limit) != 0)
        return 2;
    if (limit.rlim_max != RLIM_INFINITY && limit.rlim_max < 256 * 1024)
        return 2;
    limit.rlim_cur = 256 * 1024;
    if (setrlimit(RLIMIT_STACK, &limit) != 0 || setrlimit(RLIMIT_CORE, &no_core) != 0)
        return 2;
    stack_limit = limit.rlim_cur;
    if (strcmp(argv[1], "bounded") == 0)
        bounded_depth = 4;
    else if (strcmp(argv[1], "exhaust") != 0)
        return 2;
    observed_sum = f(1);
    trace_phase = 3;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_RECURSION_RESULT */
    printf("bounded sum = %u\n", observed_sum);
    return observed_sum == 10 ? 0 : 1;
}

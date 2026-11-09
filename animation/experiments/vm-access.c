#define _GNU_SOURCE
/* The collector must not read the cold target page before the program accesses it. */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

volatile int trace_phase;
volatile unsigned char *target;
volatile unsigned char value;
long page_size;
long before_faults, after_faults;

int main(int argc, char **argv)
{
    struct rlimit no_core = {0, 0};
    struct rusage usage;
    if (argc != 2 || setrlimit(RLIMIT_CORE, &no_core) != 0)
        return 2;
    page_size = sysconf(_SC_PAGESIZE);
    unsigned char *base = mmap(NULL, 2 * (size_t)page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
        return 3;
    int writing = 0;
    if (strcmp(argv[1], "resident-a") == 0 || strcmp(argv[1], "resident-b") == 0) {
        target = base + 0x2D;
        *target = strcmp(argv[1], "resident-a") == 0 ? 0x7B : 0xA6;
    } else if (strcmp(argv[1], "cold") == 0) {
        target = base + page_size + 0x2D;
    } else if (strcmp(argv[1], "readonly") == 0) {
        target = base + 0x2D;
        *target = 0xAA;
        if (mprotect(base, (size_t)page_size, PROT_READ) != 0)
            return 4;
        writing = 1;
    } else if (strcmp(argv[1], "unmapped") == 0) {
        target = base + page_size + 0x2D;
        if (munmap(base + page_size, (size_t)page_size) != 0)
            return 5;
    } else {
        return 6;
    }
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return 7;
    before_faults = usage.ru_minflt;
    trace_phase = 1;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_VM_BEFORE */
    if (writing)
        *target = 0xFF;
    else
        value = *target;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return 8;
    after_faults = usage.ru_minflt;
    trace_phase = 2;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_VM_AFTER */
    printf("address=%p value=%02X page_size=%ld minor_delta=%ld\n", (void *)target, (unsigned)value, page_size, after_faults - before_faults);
    return 0;
}

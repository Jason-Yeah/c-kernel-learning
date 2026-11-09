#include "os-demo.h"

int os_data = 10;
int os_bss;
const char os_message[] = "OS demo";

__attribute__((noinline)) int os_add(int left, int right)
{
    return left + right;
}

/* A deliberate, safe x86-64 register-width experiment, not ordinary C syntax. */
__attribute__((noinline)) uint64_t os_width(void)
{
    uint64_t value;
    __asm__ volatile("movabs $0xffffffffffffffff, %%rax\n\t"
                     "mov $5, %%eax\n\t"
                     : "=a"(value));
    return value;
}

__attribute__((noinline)) int os_equal(int left, int right)
{
    return left == right;
}

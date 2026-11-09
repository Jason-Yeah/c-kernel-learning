/* Controlled UB demonstration. Run only these fixed local cases under GDB.
 * This explicit byte loop illustrates strcpy semantics, not glibc's implementation.
 */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

const unsigned char *input;
size_t input_length;
volatile size_t copy_index;
volatile int trace_phase;
static unsigned char broken[27];

__attribute__((noinline)) static void copy_into_buf(void)
{
    _Alignas(16) unsigned char buf[8] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
    copy_index = 0;
    trace_phase = 1;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_BUFFER_READY */
    for (;;) {
        buf[copy_index] = input[copy_index];
        trace_phase = 2;
        __asm__ volatile ("nop" ::: "memory"); /* TRACE_BUFFER_BYTE */
        if (input[copy_index] == 0)
            break;
        ++copy_index;
    }
    trace_phase = 3;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_BUFFER_DONE */
}

int main(int argc, char **argv)
{
    struct rlimit no_core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &no_core) != 0 || argc != 2)
        return 2;
    if (strcmp(argv[1], "safe") == 0) {
        input = (const unsigned char *)"CSAPP!!";
        input_length = 8;
    } else if (strcmp(argv[1], "edge") == 0) {
        input = (const unsigned char *)"CSAPP!!!";
        input_length = 9;
    } else if (strcmp(argv[1], "broken") == 0) {
        memset(broken, 0x41, 24);
        broken[24] = 0xAD;
        broken[25] = 0xDE;
        broken[26] = 0;
        input = broken;
        input_length = sizeof broken;
    } else {
        return 2;
    }
    copy_into_buf();
    trace_phase = 4;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_BUFFER_RETURNED */
    puts("normal return");
    return 0;
}

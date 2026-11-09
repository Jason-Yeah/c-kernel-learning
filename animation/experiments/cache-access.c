/* GDB observes loads and bytes, NOT hardware Cache tags, LRU or hit/miss. */
#include <stdio.h>

_Alignas(64) unsigned char memory[256];
const unsigned offsets[8] = {0x00, 0x03, 0x20, 0x00, 0x40, 0x20, 0x08, 0x0C};
volatile unsigned step;
volatile unsigned offset;
volatile unsigned char value;
volatile unsigned char sink;
const volatile unsigned char *target;
volatile int trace_phase;

int main(void)
{
    for (unsigned i = 0; i < sizeof memory; ++i)
        memory[i] = (unsigned char)(7 * i + 0x11);
    trace_phase = 1;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_CACHE_READY */
    for (step = 0; step < 8; ++step) {
        offset = offsets[step];
        target = &memory[offset];
        value = *target;
        sink = value;
        trace_phase = 2;
        __asm__ volatile ("nop" ::: "memory"); /* TRACE_CACHE_READ */
    }
    printf("8 loads; last byte = %02X\n", (unsigned)sink);
    return sink == 0x65 ? 0 : 1;
}

/* Reviewed fixed experiment: object representation and explicit BE encoding.
 * Linux x86-64, 8-bit bytes. No type punning through uint32_t pointers.
 * Observer checkpoints are not part of the byte-order mechanism.
 */
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

_Static_assert(CHAR_BIT == 8 && sizeof(uint32_t) == 4, "four 8-bit bytes required");
uint32_t word = UINT32_C(0x12345678);
unsigned char be_encoded[4] = {0xCC, 0xCC, 0xCC, 0xCC};
unsigned char probe_byte;
unsigned read_index, encode_index;
uint32_t native_read, raw_read, decoded_be;
volatile unsigned trace_phase;

__attribute__((noinline)) static void checkpoint(unsigned phase)
{
    trace_phase = phase;
    __asm__ volatile("" ::: "memory");
}

int main(void)
{
    const struct rlimit no_core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &no_core) != 0) return 2;
    const unsigned char *view = (const unsigned char *)&word;
    checkpoint(1);
    for (read_index = 0; read_index < sizeof word; ++read_index) {
        probe_byte = view[read_index];
        checkpoint(2);
    }
    for (encode_index = 0; encode_index < sizeof be_encoded; ++encode_index) {
        be_encoded[encode_index] = (unsigned char)(word >> (24 - 8 * encode_index));
        checkpoint(3);
    }
    memcpy(&native_read, &word, sizeof native_read);
    checkpoint(4);
    memcpy(&raw_read, be_encoded, sizeof raw_read);
    checkpoint(5);
    decoded_be = ((uint32_t)be_encoded[0] << 24)
               | ((uint32_t)be_encoded[1] << 16)
               | ((uint32_t)be_encoded[2] << 8)
               | (uint32_t)be_encoded[3];
    checkpoint(6);
    printf("word=%08" PRIX32 " native=%08" PRIX32 " raw=%08" PRIX32 " decoded=%08" PRIX32 "\n",
           word, native_read, raw_read, decoded_be);
    return native_read == word && decoded_be == word && raw_read == UINT32_C(0x78563412) ? 0 : 1;
}

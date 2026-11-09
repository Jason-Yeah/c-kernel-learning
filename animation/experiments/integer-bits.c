/* Reviewed, fixed C11 experiment: no signed overflow or invalid shift is run.
 * Linux x86-64; the observer checkpoints do not perform the operations for us. */
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

_Static_assert(CHAR_BIT == 8 && sizeof(int) == 4, "requires 8-bit bytes / int32");
_Static_assert(UINT_MAX == UINT32_MAX, "requires 32-bit unsigned int");

uint32_t bits = UINT32_C(0xB6), mask = UINT32_C(0x0F);
uint8_t small = UINT8_C(0xB6);
uint32_t r_and, r_or, r_xor, r_not, r_left, r_right, guarded_shift;
int r_logical, r_logical_not, promoted_not, small_shift;
uint8_t narrow_not;
unsigned shift_count = 32;
int shift_rejected;

uint8_t u8 = 214, magnitude = 42, inverted8, negated8;
int8_t s8 = -42, small_max = INT8_MAX, small_min = INT8_MIN;
int same_encoding;
uint16_t zero_extended;
int16_t sign_extended;
uint32_t wide_unsigned = UINT32_C(0x1234);
uint8_t truncated;
uint32_t wrap_lhs = UINT32_MAX, wrapped;
uint8_t u8_max = UINT8_MAX, wrapped8;
int promoted_256;
int32_t signed_lhs = INT32_MAX, signed_result;
int64_t wide_sum;
int signed_rejected, promoted_add, promoted_negmin;

__attribute__((noinline)) static void checkpoint(unsigned phase) {
    __asm__ volatile("" : : "r"(phase) : "memory");
}

int main(void) {
    struct rlimit core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &core) != 0) return 2;
    checkpoint(1);
    r_and = bits & mask;
    r_logical = bits && mask;
    r_logical_not = !bits;
    checkpoint(2);
    r_or = bits | mask;
    checkpoint(3);
    r_xor = bits ^ mask;
    checkpoint(4);
    r_not = ~bits;
    checkpoint(5);
    promoted_not = ~small; /* small promotes to int before ~. */
    narrow_not = (uint8_t)promoted_not; /* modulo 256 conversion. */
    small_shift = small << 8; /* promoted int width is 32, not 8. */
    checkpoint(6);
    r_left = bits << 2;
    checkpoint(7);
    r_right = bits >> 2;
    checkpoint(8);
    shift_rejected = shift_count >= sizeof(+bits) * CHAR_BIT;
    if (!shift_rejected) guarded_shift = bits >> shift_count;
    checkpoint(9);

    same_encoding = memcmp(&u8, &s8, sizeof(u8)) == 0;
    checkpoint(10);
    inverted8 = (uint8_t)(~(uint32_t)magnitude);
    checkpoint(11);
    negated8 = (uint8_t)((uint32_t)inverted8 + UINT32_C(1));
    checkpoint(12);
    zero_extended = (uint16_t)u8;
    sign_extended = (int16_t)s8;
    checkpoint(13);
    truncated = (uint8_t)wide_unsigned;
    checkpoint(14);
    wrapped = wrap_lhs + UINT32_C(1);
    promoted_256 = u8_max + 1;
    wrapped8 = (uint8_t)promoted_256;
    checkpoint(15);
    wide_sum = (int64_t)signed_lhs + INT64_C(1);
    if (wide_sum >= INT32_MIN && wide_sum <= INT32_MAX)
        signed_result = (int32_t)wide_sum;
    else
        signed_rejected = 1; /* No overflowing int32_t addition is executed. */
    checkpoint(16);
    promoted_add = small_max + 1;
    promoted_negmin = -small_min;
    checkpoint(17);

    printf("AND=%08" PRIX32 " OR=%08" PRIX32 " XOR=%08" PRIX32
           " NOT=%08" PRIX32 " LEFT=%08" PRIX32 " RIGHT=%08" PRIX32 "\n",
           r_and, r_or, r_xor, r_not, r_left, r_right);
    printf("logical=%d promoted_not=%d narrow_not=%u small_shift=%d rejected_shift=%d\n",
           r_logical, promoted_not, (unsigned)narrow_not, small_shift, shift_rejected);
    printf("u8=%u s8=%d zero=%04X sign=%04X truncated=%02X wrap=%" PRIu32
           " wide=%" PRId64 " rejected_signed=%d promoted_add=%d negmin=%d\n",
           (unsigned)u8, (int)s8, (unsigned)zero_extended,
           (unsigned)(uint16_t)sign_extended, (unsigned)truncated, wrapped,
           wide_sum, signed_rejected, promoted_add, promoted_negmin);
    return !(r_and == 6 && r_or == 191 && r_xor == 185 && r_not == UINT32_C(0xFFFFFF49)
             && r_left == 728 && r_right == 45 && r_logical == 1 && r_logical_not == 0
             && promoted_not == -183 && narrow_not == 73 && small_shift == 46592
             && shift_rejected == 1 && guarded_shift == 0 && same_encoding == 1
             && inverted8 == 213 && negated8 == 214 && zero_extended == 214
             && sign_extended == -42 && truncated == 52 && wrapped == 0
             && promoted_256 == 256 && wrapped8 == 0 && wide_sum == INT64_C(2147483648)
             && signed_rejected == 1 && signed_result == 0
             && promoted_add == 128 && promoted_negmin == 128);
}

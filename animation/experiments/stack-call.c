/* Safe, single-threaded experiment for GDB-driven animation.
 * Compilation and capture: npm run capture:stack
 * No buffer overflow, process injection or system-wide setting changes.
 */
__attribute__((noinline))
int add(int a, int b)
{
    int sum = a + b;
    return sum; /* TRACE_ADD_RESULT */
}

__attribute__((noinline))
int caller(int n)
{
    int local = n;
    int result = add(local, 2); /* TRACE_CALL_ADD */
    return result;
}

int main(void)
{
    int result = caller(3); /* TRACE_CALL_CALLER */
    return result == 5 ? 0 : 1;
}

#include <stdio.h>
#include <sys/resource.h>

volatile int inputs[2] = {3, 2};
volatile int observed;

__attribute__((noinline)) int calculate(void)
{
    int left = inputs[0];
    int right = inputs[1];
    int result = left + right;
    observed = result;
    return result;
}

int main(void)
{
    struct rlimit core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &core) != 0)
        return 1;
    int result = calculate();
    printf("inputs=%d,%d observed=%d return=%d\n",
           inputs[0], inputs[1], observed, result);
    return result == 5 && observed == 5 ? 0 : 2;
}

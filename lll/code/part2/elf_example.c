#include <stdio.h>

int g_init = 42;
int g_uninit;
const char *msg = "hello";

int add(int a, int b) {
    return a + b;
}

int main() {
    printf("%s %d %d %d\n", msg, g_init, g_uninit, add(1,2));
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int global_var = 100; // 数据段 (Data Segment)

int main() {
    void *heap_var = malloc(1024); // 堆 (Heap)
    int stack_var = 10;            // 栈 (Stack)

    printf("进程 PID: %d\n", getpid());
    printf("代码段地址: %p\n", (void*)main);
    printf("数据段地址: %p\n", (void*)&global_var);
    printf("堆地址:     %p\n", heap_var);
    printf("栈地址:     %p\n", (void*)&stack_var);

    while(1) sleep(1); // 让程序持续运行，方便我们观察
    return 0;
}

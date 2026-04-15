#include <stdio.h>
#include <pthread.h>

// 关键点：不要重新声明一遍函数签名，直接给已有的符号加上 weak 属性
__attribute__ ((weak)) extern int pthread_create(pthread_t *, const pthread_attr_t *, 
                                                void *(*)(void *), void *);

int main() {
    // 检查符号地址是否被链接器填充
    if (pthread_create) {
        printf("检测到多线程库：当前模式为 [多线程]\n");
    } else {
        printf("未检测到多线程库：当前模式为 [单线程]\n");
    }
    return 0;
}
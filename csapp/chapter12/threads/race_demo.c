#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* 对等线程例程 */
void *thread(void *vargp) {
    /* 关键错误点：子线程去读主线程里 i 的地址 */
    int myid = *((int *)vargp); 
    free(vargp);
    printf("Hello from thread %d\n", myid);
    return NULL;
}

int main() {
    pthread_t tid[4];
    int i, *ptr;

    /* 创建 4 个线程 */
    for (i = 0; i < 4; i++) {
        /* 核心错误：传递了本地变量 i 的地址 &i */
        ptr = malloc(sizeof (int));
        *ptr = i;
        pthread_create(&tid[i], NULL, thread, ptr); 
    }

    /* 等待所有线程结束 */
    for (i = 0; i < 4; i++) {
        pthread_join(tid[i], NULL);
    }

    return 0;
}

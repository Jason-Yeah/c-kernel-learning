#include <stdio.h>
#include <pthread.h>
#include <semaphore.h> // 1. 引入信号量库

int cnt = 0; 
sem_t mutex; // 2. 定义信号量变量

void *thread_routine(void *vargp) {
    int i;
    for (i = 0; i < 100; i++) {
        /* 3. P 操作 (Wait)：如果信号量 > 0，则减 1 并进入；否则阻塞 */
        sem_wait(&mutex); 
        
        cnt++; 
        
        /* 4. V 操作 (Post)：将信号量加 1，如果有等待者则唤醒 */
        sem_post(&mutex); 
    }
    return NULL;
}

int main() {
    pthread_t tid1, tid2;

    /* 5. 初始化信号量
       参数 2：0 表示线程间共享，非 0 表示进程间共享
       参数 3：初始值设为 1（表示只有一个坑位，即互斥） */
    sem_init(&mutex, 0, 1); 

    pthread_create(&tid1, NULL, thread_routine, NULL);
    pthread_create(&tid2, NULL, thread_routine, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    /* 6. 清理信号量 */
    sem_destroy(&mutex);

    printf("最终结果 cnt = %d\n", cnt);
    return 0;
}

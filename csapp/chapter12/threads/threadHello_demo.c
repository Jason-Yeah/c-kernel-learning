#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>  // 标准线程库，代替了 csapp.h 里的封装
#include <unistd.h>   // 提供 sleep 函数

// 线程函数：新线程会从这里开始执行
// 必须返回 void*，必须接收 void* 参数
void *thread_routine(void *vargp) {
    sleep(3);
    char *msg = (char *)vargp;
    printf("Hello from the new thread! (ID: %lu)\n", (unsigned long)pthread_self());
    printf("Thead say: %s\n", msg);
    return NULL; // 线程结束，返回空指针
}

int main() {
    pthread_t tid; // 线程 ID (工牌)
    char *msg = "JasonYe";

    printf("ID: %lu\n", (unsigned long)pthread_self());
    printf("Main thread: Creating a peer thread...\n");

    /* * 原生创建函数 pthread_create:
     * 1. &tid: 存储新线程 ID 的地址
     * 2. NULL: 线程属性（使用默认值）
     * 3. thread_routine: 线程要运行的函数名
     * 4. NULL: 传递给该函数的参数
     */
    if (pthread_create(&tid, NULL, thread_routine, msg) != 0) {
        perror("pthread_create failed");
        exit(1);
    }

    /* * 等待线程结束:
     * 主线程会在这里挂起，直到 tid 对应的线程执行完毕。
     * 如果不加这一行，主线程执行 exit(0) 会直接杀掉整个进程，
     * 导致新线程还没来得及打印就挂了。
     */
    pthread_join(tid, NULL); 

    printf("Main thread: Peer thread has finished. Exiting.\n");
    exit(0); // 终止进程
}

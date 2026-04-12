#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* 线程函数原型 */
void *thread_routine(void *vargp);

int main(int argc, char **argv) {
    int listenfd;
    struct sockaddr_in serveraddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    /* 1. 对应 Open_listenfd: 创建并配置监听套接字 */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(argv[1]));

    bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, 1024);

    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t clientlen = sizeof(clientaddr);
        pthread_t tid;

        /* 2. 对应 Malloc(sizeof(int)): 为每个 connfd 分配独立内存 */
        /* 这是为了防止主线程下一次 Accept 时覆盖掉当前的 connfd */
        int *connfdp = malloc(sizeof(int)); 

        /* 3. 对应 Accept: 阻塞等待新连接 */
        *connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

        /* 4. 对应 Pthread_create: 创建服务线程 */
        /* 传给线程的参数就是刚刚 malloc 的地址 */
        pthread_create(&tid, NULL, thread_routine, connfdp);
    }
}

/* 线程服务例程 (Thread routine) */
void *thread_routine(void *vargp) {
    /* 5. 获取 connfd 并释放主线程申请的内存 (对应 Free(vargp)) */
    int connfd = *((int *)vargp);
    free(vargp);

    /* 6. 对应 Pthread_detach: 设置为分离状态，结束后自动回收 */
    pthread_detach(pthread_self());

    /* 7. 对应 echo(connfd): 简单的回显逻辑 */
    char buf[1024];
    ssize_t n;
    while ((n = recv(connfd, buf, 1024, 0)) > 0) {
        send(connfd, buf, n, 0);
    }

    /* 8. 对应 Close(connfd): 关闭连接 */
    close(connfd);
    return NULL;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>

#define MAXLINE 8192

/* 辅助函数：打开监听描述符 */
int open_listenfd(char *port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; 
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break; 
        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p) return -1;
    if (listen(listenfd, 1024) < 0) {
        close(listenfd);
        return -1;
    }
    return listenfd;
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    fd_set read_set, ready_set; // read_set用于维护所有要监控的fd，ready_set用于select返回
    int fd_list[FD_SETSIZE];    // 用于记录当前有哪些连接，方便遍历
    int max_idx = -1;           // fd_list中当前最大索引

    if (argc != 2) {
        fprintf(stderr, "用法: %s <端口>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);
    
    // 初始化工作
    FD_ZERO(&read_set);              // 清空位图
    FD_SET(listenfd, &read_set);     // 把监听描述符加入监控名单
    
    for (int i = 0; i < FD_SETSIZE; i++) fd_list[i] = -1; // 初始化记录表

    printf("Select并发服务器启动，端口: %s\n", argv[1]);

    while (1) {
        ready_set = read_set; // 必须备份，因为select会修改传入的set
        
        /* 作用：阻塞在这里，直到监听的fd中有一个或多个变为“可读” */
        if (select(FD_SETSIZE, &ready_set, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        /* 情况A：监听描述符有动静，说明有新客人来了 */
        if (FD_ISSET(listenfd, &ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
            
            // 将新的连接加入监控位图
            FD_SET(connfd, &read_set);
            
            // 存入列表以便后续遍历查看数据
            int i;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (fd_list[i] < 0) {
                    fd_list[i] = connfd;
                    break;
                }
            }
            if (i > max_idx) max_idx = i;
            printf("收到新连接，描述符 fd: %d\n", connfd);
        }

        /* 情况B：检查所有已连接的客户端描述符，看谁发了数据 */
        for (int i = 0; i <= max_idx; i++) {
            int fd = fd_list[i];
            if (fd > 0 && FD_ISSET(fd, &ready_set)) {
                char buf[MAXLINE];
                ssize_t n = read(fd, buf, MAXLINE);
                
                if (n > 0) { // 收到数据
                    printf("从 fd %d 收到 %ld 字节内容\n", fd, n);
                    write(fd, buf, n); // 原样回传（Echo）
                } else { // n == 0 代表客户端断开，或者 n < 0 报错
                    printf("连接 fd %d 已关闭\n", fd);
                    close(fd);
                    FD_CLR(fd, &read_set); // 必须从监控名单中剔除！
                    fd_list[i] = -1;
                }
            }
        }
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXLINE 8192

// 模拟 CSAPP 的封装逻辑：打开一个监听描述符
int open_listenfd(char *port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             // 接受 TCP 连接
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // 在任何 IP 上接受连接
    getaddrinfo(NULL, port, &hints, &listp);     // 查表

    for (p = listp; p; p = p->ai_next) {
        // 1. 买手机
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;
        
        // 消除“Address already in use”错误
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        // 2. 定摊位（绑定端口）
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break; 
        
        close(listenfd); // 绑定失败，换个地址再试
    }

    freeaddrinfo(listp);
    if (!p) return -1;

    // 3. 开启铃声（开始监听）
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
    char client_hostname[MAXLINE], client_port[MAXLINE];
    char buf[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "用法: %s <端口>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);
    printf("服务器启动，监听端口 %s...\n", argv[1]);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        // 4. 接电话（生成专职服务员 connfd）
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

        // 查一下是谁连了我
        getnameinfo((struct sockaddr *)&clientaddr, clientlen, 
                    client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("收到来自 %s:%s 的连接\n", client_hostname, client_port);

        // 5. 聊天：把客户端发来的东西再发回去
	// 5. 简单的 Web 响应逻辑
	// 5. Web 响应逻辑
ssize_t n;
if ((n = read(connfd, buf, MAXLINE)) > 0) {
    // 构造一个最兼容的 HTTP 响应
    // 注意：\r\n\r\n 是报文头和正文之间的“分界线”，必须存在
    char *response = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html; charset=utf-8\r\n"
                     "Content-Length: 38\r\n"
                     "Connection: close\r\n"
                     "\r\n" 
                     "<html><body>Hello from WSL!</body></html>";

    write(connfd, response, strlen(response));
}
// 发送完必须立即 close，浏览器才会停止“转圈”并渲染页面
close(connfd);

	printf("连接关闭。\n");
        close(connfd); // 挂断电话
    }
}

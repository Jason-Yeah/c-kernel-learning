#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXLINE 8192

int main(int argc, char **argv) {
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];

    if (argc != 2) return 1;

    // 1. 设置筛选条件 (hints)
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       // 只想要 IPv4
    hints.ai_socktype = SOCK_STREAM; // 只想要 TCP

    // 2. 调用函数：查表
    // 结果会被存入 listp 指向的链表中
    if (getaddrinfo(argv[1], NULL, &hints, &listp) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    // 3. 遍历链表 (ai_next)
    for (p = listp; p != NULL; p = p->ai_next) {
        // 使用 getnameinfo 将二进制 IP 变回人类可读的字符串
        // 这比你手动处理 sockaddr 结构体要优雅得多
        getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, NI_NUMERICHOST);
        printf("%s\n", buf);
    }

    // 4. 释放内存：查表时内核在堆区开了空间，一定要还回去
    freeaddrinfo(listp);

    return 0;
}

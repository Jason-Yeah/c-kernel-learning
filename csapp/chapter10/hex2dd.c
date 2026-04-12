#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    struct in_addr inaddr; // 用于存储二进制 IP 的结构体
    uint32_t addr;         // 存储十六进制数值
    char buf[INET_ADDRSTRLEN]; // 存储转换后的点分十进制字符串

    if (argc != 2) {
        fprintf(stderr, "用法: %s <hex_address>\n", argv[0]);
        exit(1);
    }

    // 1. 将输入的十六进制字符串转为数值
    // 使用 sscanf 解析 "0x8002c2f2"
    sscanf(argv[1], "%x", &addr);

    // 2. 将数值转为网络字节序并存入结构体
    // 题目输入的 hex 通常直接对应网络字节序的排列
    inaddr.s_addr = htonl(addr); 

    // 3. 将二进制转换为点分十进制字符串
    if (inet_ntop(AF_INET, &inaddr, buf, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop 转换出错");
        exit(1);
    }

    // 4. 打印结果
    printf("%s\n", buf);

    return 0;
}

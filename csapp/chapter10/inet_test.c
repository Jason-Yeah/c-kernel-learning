#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>  // 核心库

int main() {
    const char *ip_str = "192.168.1.10"; // 人看的字符串
    struct in_addr server_addr;          // 机器存 IP 的结构体
    char back_to_str[INET_ADDRSTRLEN];   // 用来存转回来的字符
    const char *res;


    // --- 第一步：Presentation to Network (pton) ---
    // 把 "192.168.1.10" 转成二进制存入 server_addr
    if (inet_pton(AF_INET, ip_str, &server_addr) <= 0) {
        perror("inet_pton 失败");
        return 1;
    }
    // 打印出机器看到的十六进制（注意：大端序存储）
    printf("字符串 [%s] 转换成十六进制为: 0x%x\n", ip_str, server_addr.s_addr);

    // --- 第二步：Network to Presentation (ntop) ---
    // 把二进制再转回人能看懂的字符串
    res = inet_ntop(AF_INET, &server_addr, back_to_str, INET_ADDRSTRLEN); 
    if (res == NULL) {
        perror("inet_ntop 失败");
        return 1;
    }
    printf("从二进制转回来的字符串为: %s\n", res);

    return 0;
}

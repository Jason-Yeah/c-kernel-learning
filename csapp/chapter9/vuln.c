#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 这是一个正常情况下永远不会被调用的函数（禁区）
void secret_backdoor() {
    printf("\n[!!!] 警告：你已绕过权限检查，进入了秘密后门！\n");
    printf("[!!!] 正在获取系统控制权...\n");
    exit(0);
}

void echo_input() {
    char buffer[16]; // 缓冲区只有 16 字节
    printf("请输入一些内容: ");
    gets(buffer);    // 危险！gets 不检查边界
    printf("你输入的是: %s\n", buffer);
}

int main() {
    echo_input();
    printf("程序正常退出。\n");
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void secret_backdoor() {
    printf("!!! 警告：你已非法闯入秘密后门 !!!\n");
    system("/bin/sh"); // 启动一个 Shell
}

void echo_input() {
    char buf[8]; // 只能存 8 个字符
    printf("请输入内容: ");
    gets(buf);   // 危险！gets 不检查长度，会一直往后写
    printf("你输入了: %s\n", buf);
}

int main() {
    echo_input();
    return 0;
}
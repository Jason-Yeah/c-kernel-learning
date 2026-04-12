#include <stdio.h>

int main() {
    FILE *fp;
    int c; // 必须用 int，不能用 char！后面会讲原因

    // 1. 打开文件
    fp = fopen("test.txt", "r");
    if (fp == NULL) {
        perror("打开文件失败");
        return -1;
    }

    // 2. 循环读取：每次读取一个字符，存入 c
    // 当 fgetc 返回 EOF 时，循环停止
    while ((c = fgetc(fp)) != EOF) {
        printf("读取到字符: %c (ASCII码: %d)\n", c, c);
    }

    // 3. 检查是读完了还是出错了
    if (feof(fp)) {
        printf("已成功到达文件末尾 (EOF)。\n");
    } else if (ferror(fp)) {
        printf("读取过程中发生错误。\n");
    }

    // 4. 关闭文件
    fclose(fp);
    return 0;
}

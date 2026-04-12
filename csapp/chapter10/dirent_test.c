#include <stdio.h>
#include <dirent.h>

int main() {
    DIR *dr = opendir("."); // 打开当前目录
    struct dirent *de;

    if (dr == NULL) return 1;

    // 循环读取，直到读完所有条目
    while ((de = readdir(dr)) != NULL) {
        printf("发现文件: %s\n", de->d_name);
    }

    closedir(dr); // 记得关闭
    return 0;
}

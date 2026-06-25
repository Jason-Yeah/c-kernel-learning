#include <stdio.h>
#include <unistd.h>

int main()
{
    int cfd1, cfd2;
    char str1[] = "111\n";
    char str2[] = "222\n";

    cfd1 = dup(1);
    cfd2 = dup2(cfd1, 7);

    // 注 printf是行缓冲模式_IOLBF遇到\n或缓冲区满了才真正write，由于下面
    // 内容没\n且也没让缓冲区满，数据停留在用户态没写入内核。
    printf("fd1 = %d, fd2 = %d", cfd1, cfd2);
    fflush(stdout); // 无论上面加不加\n，强制刷新缓冲区就可输出

    write(cfd1, str1, sizeof(str1));
    write(cfd2, str2, sizeof(str2));

    close(cfd1);
    close(cfd2);

    write(1, str1, sizeof(str1));
    close(1);
    write(1, str2, sizeof(str2));

    return 0;
}
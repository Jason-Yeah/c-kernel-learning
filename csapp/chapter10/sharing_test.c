#include "csapp.h" // 包含 CSAPP 的 RIO 库和标准头文件

int main() {
    int fd;
    char c;

    // 1. 打开一个已经存在的文件（假设里面内容是 "123456789"）
    fd = Open("numbers.txt", O_RDONLY, 0);

    // 2. 创建子进程
    if (Fork() == 0) {
        /* 子进程逻辑 */
        sleep(1); // 确保父进程先读
        Read(fd, &c, 1); // 从 fd 读取 1 个字节
        printf("子进程读到了: %c\n", c);
        exit(0);
    }

    /* 父进程逻辑 */
    Read(fd, &c, 1); // 父进程先读第 1 个字节
    printf("父进程读到了: %c\n", c);
    
    Wait(NULL); // 等待子进程结束
    exit(0);
}

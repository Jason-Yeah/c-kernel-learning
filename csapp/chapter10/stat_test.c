#include <sys/stat.h>
#include <stdio.h>

int main() {
    struct stat buf;
    
    if (stat("test.txt", &buf) == 0) {
        printf("文件大小: %ld 字节\n", buf.st_size);
        
        if (S_ISDIR(buf.st_mode)) {
            printf("这是一个目录\n");
        } else if(S_ISREG(buf.st_mode)) {
            printf("这是一个普通文件\n");
        }
    }
    return 0;
}

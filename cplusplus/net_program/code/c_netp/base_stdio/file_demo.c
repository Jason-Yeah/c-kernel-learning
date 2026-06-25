#include <stdio.h>
#include <string.h>

#define BUF_SIZE 16

struct _MY_FILE
{
    char r_buf[BUF_SIZE];
    char w_buf[BUF_SIZE];
    int r_pos;
    int w_pos;
};

typedef struct _MY_FILE MY_FILE;

int main(void)
{
    MY_FILE fp = {0};
    
    strcpy(fp.w_buf, "LOG:start");
    fp.w_pos = strlen("LOG:start");

    printf("content: %s\n", fp.w_buf);
    printf("w_pos: %d\n", fp.w_pos);

    printf("\n【错误】试图从写缓冲区读取：[%.*s]\n", 
           5, fp.w_buf);
    
    strcpy(fp.r_buf, "port=8080");  // 假设读到了配置
    fp.r_pos = 0;

    printf("读缓冲区内容: [%s]\n", fp.r_buf);
    printf("读指针位置: %d\n", fp.r_pos);  // 输出: 0

    return 0;

}
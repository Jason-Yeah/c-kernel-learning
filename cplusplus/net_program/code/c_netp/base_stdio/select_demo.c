#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int main()
{
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(0, &read_set);

    // 谁就绪了就把谁设为1，其他全0
    int ret = select(1, &read_set, 0, 0, 0);

    if (ret == -1) perror("select error");
    else if (ret == 0) printf("timeout\n");
    else
    {
        if (FD_ISSET(0, &read_set))
        {
            char buf[20];
            read(0, buf, sizeof(buf));
            printf("你输入了 %s", buf);
        }
    }
    return 0;
}
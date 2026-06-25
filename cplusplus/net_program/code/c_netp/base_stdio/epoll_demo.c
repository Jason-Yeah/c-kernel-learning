#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    int epfd = epoll_create(1);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);
    
    printf("%d\n", epfd);
    struct epoll_event ready_evs[10];

    while (1)
    {
        int n = epoll_wait(epfd, ready_evs, 10, -1);

        for (int i = 0; i < n; ++ i)
            if (ready_evs[i].data.fd == 0)
            {
                char buf[100];
                read(0, buf, sizeof(buf));
                printf("你输入了: %s", buf);
            }
    }
    close(epfd);
    return 0;
}
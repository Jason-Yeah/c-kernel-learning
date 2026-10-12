#include <cstdio>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    int a[2], b[2];

    pipe(a);
    pipe(b);

    // 只向管道 b 写数据
    write(b[1], "hello", 5);
    /*
    pollfd fds[2] = {{a[0], POLLIN, 0}, {b[0], POLLIN, 0}};

    // 等待两个管道中有可读事件
    int n = poll(fds, 2, -1);

    if (n < 0)
        return 1;

    printf("ready count = %d\n", n);

    for (auto &event : fds)
    {
        if (event.revents & POLLIN)
        {
            char buf[16]{};

            ssize_t count = read(event.fd, buf, sizeof(buf));

            if (count > 0)
                printf("fd=%d data=%.*s\n", event.fd, static_cast<int>(count),
                       buf);
        }
    }
    */

    // epoll
    int epfd = epoll_create1(EPOLL_CLOEXEC);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = 5;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 5, &ev);

    epoll_event ready[10];
    int n = epoll_wait(epfd, ready, 10, -1);
    int i{};
    for (i = 0; i < n; ++i)
    {
        int fd = ready[i].data.fd;
        if (ready[i].events & EPOLLIN)
        {
            char buf[16]{};
            auto cnt = read(fd, buf, sizeof(buf));

            if (cnt > 0)
            {
                // printf("fd=%d data=%.*s\n", fd, static_cast<int>(cnt), buf);
                std::cout << "fd = " << fd << " data = ";
                std::cout.write(buf, cnt);
                std::cout << std::endl;
            }
        }
    }

    close(epfd);
    close(a[0]);
    close(a[1]);
    close(b[0]);
    close(b[1]);
}
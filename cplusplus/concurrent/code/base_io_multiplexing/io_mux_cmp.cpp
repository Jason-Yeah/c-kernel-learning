#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

void die(const char *op)
{
    perror(op);
    std::exit(1);
}

int make_listener()
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
        die("socket");
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        die("setsockopt");
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(9090);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, reinterpret_cast<sockaddr *>(&a), sizeof a) < 0)
        die("bind");
    if (listen(fd, 128) < 0)
        die("listen");
    return fd;
}

template <class F> void accept_available(int listener, F add_client)
{
    for (;;)
    {
        int fd =
            accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd >= 0)
        {
            std::cout << "accepted fd=" << fd << '\n' << std::flush;
            add_client(fd);
            continue;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        perror("accept4");
        return;
    }
}

// true: connection remains open; false: caller should unregister and close.
bool read_available(int fd)
{
    char buf[1024];
    for (;;)
    {
        ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n > 0)
        {
            std::cout << "fd=" << fd << " received " << n << " bytes: ";
            std::cout.write(buf, static_cast<std::streamsize>(n));
            std::cout << '\n' << std::flush;
            continue;
        }
        if (n == 0)
        {
            std::cout << "fd=" << fd << " EOF\n" << std::flush;
            return false;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
        perror("recv");
        return false;
    }
}

void run_select(int listener)
{
    if (listener >= FD_SETSIZE)
    {
        std::fprintf(stderr, "listener fd exceeds FD_SETSIZE\n");
        std::exit(1);
    }
    std::vector<int> clients;
    for (;;)
    {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(listener, &reads);
        int maxfd = listener;
        for (int fd : clients)
        {
            FD_SET(fd, &reads);
            maxfd = std::max(maxfd, fd);
        }
        int n = select(maxfd + 1, &reads, nullptr, nullptr, nullptr);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("select");
        }

        if (FD_ISSET(listener, &reads))
        {
            accept_available(listener,
                             [&](int fd)
                             {
                                 if (fd >= FD_SETSIZE)
                                 {
                                     close(fd);
                                     return;
                                 }
                                 clients.push_back(fd);
                             });
        }
        for (std::size_t i = 0; i < clients.size();)
        {
            int fd = clients[i];
            if (FD_ISSET(fd, &reads) && !read_available(fd))
            {
                close(fd);
                clients.erase(clients.begin() + i);
            }
            else
                ++i;
        }
    }
}

void run_poll(int listener)
{
    std::vector<pollfd> fds{{listener, POLLIN, 0}};
    for (;;)
    {
        int n = poll(fds.data(), fds.size(), -1);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("poll");
        }
        if (fds[0].revents & POLLIN)
        {
            accept_available(listener,
                             [&](int fd) { fds.push_back({fd, POLLIN, 0}); });
        }

        for (std::size_t i = 1; i < fds.size();)
        {
            short rev = fds[i].revents;
            if (rev & (POLLIN | POLLHUP | POLLERR | POLLNVAL))
            {
                int fd = fds[i].fd;
                if ((rev & POLLNVAL) || !read_available(fd) || (rev & POLLERR))
                {
                    close(fd);
                    fds.erase(fds.begin() + i);
                    continue;
                }
            }
            ++i;
        }
    }
}

void run_epoll(int listener)
{
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0)
        die("epoll_create1");

    auto add = [&](int fd)
    {
        epoll_event ev{};
        ev.events = EPOLLIN; // level-triggered by default
        ev.data.fd = fd;     // user data returned by epoll_wait
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            perror("epoll_ctl ADD");
            if (fd == listener)
                std::exit(1);
            close(fd);
        }
    };

    add(listener);
    epoll_event events[64];
    for (;;)
    {
        int n = epoll_wait(epfd, events, 64, -1);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            die("epoll_wait");
        }
        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            if (fd == listener)
            {
                accept_available(listener, add);
            }
            else if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR))
            {
                bool alive = read_available(fd);
                if (!alive || (events[i].events & EPOLLERR))
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: %s select|poll|epoll\n", argv[0]);
        return 2;
    }
    int listener = make_listener();
    std::cout << "listening on 127.0.0.1:9090\n" << std::flush;
    if (std::strcmp(argv[1], "select") == 0)
        run_select(listener);
    else if (std::strcmp(argv[1], "poll") == 0)
        run_poll(listener);
    else if (std::strcmp(argv[1], "epoll") == 0)
        run_epoll(listener);
    else
    {
        std::fprintf(stderr, "unknown mode\n");
        close(listener);
        return 2;
    }
}

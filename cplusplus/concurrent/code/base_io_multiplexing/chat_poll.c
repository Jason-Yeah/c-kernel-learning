#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 64
#define LINE_SIZE 2048

static int port_arg(const char *s, unsigned short *out)
{
    char *e;
    unsigned long n = strtoul(s, &e, 10);

    if (!s[0] || *e || n > 65535)
        return -1;

    *out = (unsigned short)n;
    return 0;
}

static void broadcast(struct pollfd *fds, int from, const char *data, size_t n)
{
    for (int i = 1; i < MAX_CLIENTS + 1; i++)
        if (fds[i].fd >= 0 && i != from)
        {
            size_t off = 0;

            while (off < n)
            {
                ssize_t w = send(fds[i].fd, data + off, n - off, MSG_NOSIGNAL);

                if (w < 0)
                {
                    if (errno == EINTR)
                        continue;

                    close(fds[i].fd);
                    fds[i].fd = -1;
                    break;
                }

                if (w == 0)
                    break;

                off += (size_t)w;
            }
        }
}

static int server(unsigned short port)
{
    int l = socket(AF_INET, SOCK_STREAM, 0);

    if (l < 0)
    {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(l, (struct sockaddr *)&a, sizeof a) < 0 || listen(l, 32) < 0)
    {
        perror("bind/listen");
        close(l);
        return 1;
    }

    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0] = (struct pollfd){l, POLLIN, 0};

    for (int i = 1; i <= MAX_CLIENTS; i++)
        fds[i] = (struct pollfd){-1, POLLIN, 0};

    printf("chat server 127.0.0.1:%u (Ctrl+C to stop)\n", port);

    for (;;)
    {
        int n = poll(fds, MAX_CLIENTS + 1, -1);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            perror("poll");
            return 1;
        }

        if (fds[0].revents & POLLIN)
        {
            int c = accept(l, NULL, NULL);

            if (c >= 0)
            {
                int i;
                for (i = 1; i <= MAX_CLIENTS; i++)
                    if (fds[i].fd < 0)
                        break;

                if (i > MAX_CLIENTS)
                {
                    close(c);
                    continue;
                }

                fds[i] = (struct pollfd){c, POLLIN, 0};

                char msg[64];
                int m = snprintf(msg, sizeof msg, "[user %d joined]\n", c);
                broadcast(fds, 0, msg, (size_t)m);
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; i++)
            if (fds[i].fd >= 0 && fds[i].revents)
            {
                int c = fds[i].fd;

                if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
                {
                    close(c);
                    fds[i].fd = -1;
                    continue;
                }

                if (fds[i].revents & POLLIN)
                {
                    char b[LINE_SIZE];
                    ssize_t r = recv(c, b, sizeof b, 0);

                    if (r <= 0)
                    {
                        close(c);
                        fds[i].fd = -1;
                        continue;
                    }

                    broadcast(fds, i, b, (size_t)r);
                }
            }
    }
}

static int client(const char *ip, unsigned short port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &a.sin_addr) != 1)
    {
        fprintf(stderr, "invalid IPv4 address\n");
        close(s);
        return 2;
    }

    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0)
    {
        perror("connect");
        close(s);
        return 1;
    }

    puts("connected; type lines, Ctrl+D to leave");

    struct pollfd p[2] = {{STDIN_FILENO, POLLIN, 0}, {s, POLLIN, 0}};
    char b[LINE_SIZE];

    for (;;)
    {
        int n = poll(p, 2, -1);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        if (p[1].revents & (POLLHUP | POLLERR))
        {
            break;
        }

        if (p[1].revents & POLLIN)
        {
            ssize_t r = recv(s, b, sizeof b, 0);

            if (r <= 0)
                break;

            size_t off = 0;
            while (off < (size_t)r)
            {
                ssize_t w = write(STDOUT_FILENO, b + off, (size_t)r - off);

                if (w < 0 && errno == EINTR)
                    continue;

                if (w <= 0)
                    break;

                off += (size_t)w;
            }
        }

        if (p[0].revents & POLLIN)
        {
            ssize_t r = read(STDIN_FILENO, b, sizeof b);

            if (r <= 0)
            {
                shutdown(s, SHUT_WR);
                p[0].fd = -1;
            }
            else
            {
                size_t off = 0;
                while (off < (size_t)r)
                {
                    ssize_t w = send(s, b + off, (size_t)r - off, MSG_NOSIGNAL);

                    if (w < 0)
                    {
                        if (errno == EINTR)
                            continue;

                        perror("send");
                        close(s);
                        return 1;
                    }

                    off += (size_t)w;
                }
            }
        }
    }

    close(s);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: chat_poll server PORT | client IPv4 PORT\n");
        return 2;
    }

    unsigned short p;
    if (port_arg(argv[argc - 1], &p) < 0)
    {
        fprintf(stderr, "invalid port\n");
        return 2;
    }

    if (!strcmp(argv[1], "server") && argc == 3)
        return server(p);

    if (!strcmp(argv[1], "client") && argc == 4)
        return client(argv[2], p);

    fprintf(stderr, "invalid arguments\n");
    return 2;
}

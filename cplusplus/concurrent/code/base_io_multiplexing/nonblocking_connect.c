#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: nonblocking_connect IPv4 PORT TIMEOUT_MS\n");
        return 2;
    }

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;

    char *end = NULL;
    unsigned long p = strtoul(argv[2], &end, 10);

    if (!argv[2][0] || *end || p > 65535)
    {
        fprintf(stderr, "invalid port\n");
        return 2;
    }

    a.sin_port = htons((unsigned short)p);

    if (inet_pton(AF_INET, argv[1], &a.sin_addr) != 1)
    {
        fprintf(stderr, "invalid IPv4 address\n");
        return 2;
    }

    end = NULL;
    long timeout = strtol(argv[3], &end, 10);

    if (!argv[3][0] || *end || timeout < 0 || timeout > 3600000)
    {
        fprintf(stderr, "invalid timeout\n");
        return 2;
    }

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    if (fd < 0)
    {
        perror("socket");
        return 1;
    }

    int rc = connect(fd, (struct sockaddr *)&a, sizeof a);

    // CORE
    if (rc == 0)
    {
        puts("connected immediately");
        close(fd);
        return 0;
    }

    if (errno != EINPROGRESS)
    {
        perror("connect");
        close(fd);
        return 1;
    }

    struct pollfd event = {fd, POLLOUT, 0};

    do
    {
        rc = poll(&event, 1, (int)timeout);
    } while (rc < 0 && errno == EINTR);

    if (rc < 0)
    {
        perror("poll");
        close(fd);
        return 1;
    }

    if (rc == 0)
    {
        fprintf(stderr, "connect timeout\n");
        close(fd);
        return 1;
    }

    int error = 0;
    socklen_t len = sizeof error;

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        perror("getsockopt SO_ERROR");
        close(fd);
        return 1;
    }

    if (error)
    {
        errno = error;
        perror("connect completion");
        close(fd);
        return 1;
    }

    puts("connected");
    close(fd);
    return 0;
}

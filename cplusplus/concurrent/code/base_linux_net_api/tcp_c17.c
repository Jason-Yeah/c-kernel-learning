#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 单连接、原始字节流 echo：特意保留 C 的显式清理方式。
int main(int argc, char **argv) {
    int listener = -1, peer = -1, result = EXIT_FAILURE;
    unsigned long port = 9000;
    if (argc > 2) { fprintf(stderr, "usage: tcp_c17 [PORT]\n"); return result; }
    if (argc == 2) {
        char *end = NULL; errno = 0; port = strtoul(argv[1], &end, 10);
        if (!argv[1][0] || argv[1][0] == '-' || *end || errno || port > 65535) {
            fprintf(stderr, "invalid port\n"); return result;
        }
    }
    listener = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listener == -1) { perror("socket"); goto cleanup; }
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) { perror("setsockopt"); goto cleanup; }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET; addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) { fprintf(stderr, "bad address\n"); goto cleanup; }
    if (bind(listener, (struct sockaddr *)&addr, sizeof addr) == -1) { perror("bind"); goto cleanup; }
    if (listen(listener, 16) == -1) { perror("listen"); goto cleanup; }
    socklen_t len = sizeof addr;
    if (getsockname(listener, (struct sockaddr *)&addr, &len) == -1) { perror("getsockname"); goto cleanup; }
    printf("PORT %u\n", (unsigned)ntohs(addr.sin_port)); fflush(stdout);
    do { peer = accept(listener, NULL, NULL); } while (peer == -1 && errno == EINTR);
    if (peer == -1) { perror("accept"); goto cleanup; }
    for (;;) {
        char buf[4096]; ssize_t n = recv(peer, buf, sizeof buf, 0);
        if (n == -1) { if (errno == EINTR) continue; perror("recv"); goto cleanup; }
        if (n == 0) break;
        size_t offset = 0;
        while (offset < (size_t)n) {
            ssize_t sent = send(peer, buf + offset, (size_t)n - offset, MSG_NOSIGNAL);
            if (sent == -1) { if (errno == EINTR) continue; perror("send"); goto cleanup; }
            if (sent == 0) { fprintf(stderr, "send made no progress\n"); goto cleanup; }
            offset += (size_t)sent;
        }
    }
    result = EXIT_SUCCESS;
cleanup:
    if (peer >= 0) close(peer);
    if (listener >= 0) close(listener);
    return result;
}

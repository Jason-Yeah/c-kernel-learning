#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int reap(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno == EINTR)
            continue;
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status))
    {
        printf("reaped child=%ld exit=%d\n", (long)pid, WEXITSTATUS(status));
        return WEXITSTATUS(status) == 0 ? 0 : 1;
    }
    if (WIFSIGNALED(status))
        printf("child terminated by signal=%d\n", WTERMSIG(status));
    return 1;
}

static int write_all(int fd, const void *data, size_t count)
{
    const char *p = data;
    while (count > 0)
    {
        ssize_t n = write(fd, p, count);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        p += n;
        count -= (size_t)n;
    }
    return 0;
}

static int read_exact(int fd, void *data, size_t count)
{
    char *p = data;
    while (count > 0)
    {
        ssize_t n = read(fd, p, count);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1; // 本实验预期完整字段；中途 EOF 也算失败。
        p += n;
        count -= (size_t)n;
    }
    return 0;
}

static int fork_demo(void)
{
    int value = 10;
    fflush(NULL); // 避免把尚未刷新的 stdio 数据复制给子进程。
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return 1;
    }
    if (pid == 0)
    {
        value = 99;
        dprintf(STDOUT_FILENO, "child pid=%ld ppid=%ld value=%d\n",
                (long)getpid(), (long)getppid(), value);
        _exit(0);
    }
    int result = reap(pid);
    printf("parent pid=%ld value=%d (private memory is unchanged)\n", (long)getpid(), value);
    return result;
}

static int exec_demo(void)
{
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return 1;
    }
    if (pid == 0)
    {
        char program[] = "/bin/echo";
        char argument[] = "hello from execv";
        char *argv[] = {program, argument, NULL};
        execv(program, argv);
        // 成功的 exec 不会执行这里。示例仅在单线程父进程下 fork。
        const char error[] = "execv failed\n";
        (void)write_all(STDERR_FILENO, error, sizeof error - 1);
        _exit(127);
    }
    return reap(pid);
}

static int wait_nonblocking_demo(void)
{
    int gate[2];
    if (pipe(gate) < 0)
    {
        perror("pipe");
        return 1;
    }
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(gate[0]);
        close(gate[1]);
        return 1;
    }
    if (pid == 0)
    {
        close(gate[1]);
        char byte;
        ssize_t n;
        do
        {
            n = read(gate[0], &byte, 1);
        } while (n < 0 && errno == EINTR);
        close(gate[0]);
        _exit(n == 0 ? 23 : 1);
    }
    close(gate[0]);
    int status;
    pid_t result;
    do
    {
        result = waitpid(pid, &status, WNOHANG);
    } while (result < 0 && errno == EINTR);
    printf("before release: waitpid(WNOHANG)=%ld (expected 0)\n", (long)result);
    close(gate[1]); // child 的 read 现在得到 EOF，随后以 23 退出。
    if (result != 0)
    {
        if (result < 0)
            (void)reap(pid);
        return 1;
    }
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
        {
            perror("waitpid");
            return 1;
        }
    }
    if (!WIFEXITED(status))
        return 1;
    printf("after release: exit=%d\n", WEXITSTATUS(status));
    return WEXITSTATUS(status) == 23 ? 0 : 1;
}

static int pipe_demo(void)
{
    int fds[2];
    if (pipe(fds) < 0)
    {
        perror("pipe");
        return 1;
    }
    fflush(NULL);


    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(fds[0]);
        close(fds[1]);
        return 1;
    }
    if (pid == 0)
    {
        close(fds[1]);
        uint32_t network_length;

        char message[128];
        if (read_exact(fds[0], &network_length, sizeof network_length) < 0)
            _exit(2);

        uint32_t length = ntohl(network_length);
        if (length > sizeof message || read_exact(fds[0], message, length) < 0)
            _exit(3);
        
        if (write_all(STDOUT_FILENO, message, length) < 0)
            _exit(4);
        close(fds[0]);
        _exit(0);
    }

    close(fds[0]);
    const char message[] = "child received framed pipe message\n";

    uint32_t length = htonl((uint32_t)(sizeof message - 1));

    int ok = write_all(fds[1], &length, sizeof length);
    
    if (ok == 0)
        ok = write_all(fds[1], message, sizeof message - 1);
    close(fds[1]);
    int status = reap(pid);
    return ok == 0 && status == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: process_basics fork|exec|wait-nohang|pipe\n");
        return 2;
    }
    if (strcmp(argv[1], "fork") == 0)
        return fork_demo();
    if (strcmp(argv[1], "exec") == 0)
        return exec_demo();
    if (strcmp(argv[1], "wait-nohang") == 0)
        return wait_nonblocking_demo();
    if (strcmp(argv[1], "pipe") == 0)
        return pipe_demo();
    fprintf(stderr, "unknown mode\n");
    return 2;
}

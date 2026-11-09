#define _GNU_SOURCE
/* Two pipe-constrained executions. Only the two letters go to stdout. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

volatile int trace_phase;
volatile int role;
pid_t child_id, fork_result, wait_result = -1;
int wait_status = -1;

__attribute__((noinline)) static void checkpoint(int phase)
{
    trace_phase = phase;
    __asm__ volatile ("nop" ::: "memory"); /* TRACE_PROCESS_EVENT */
}
static void send_byte(int fd, char byte)
{
    if (write(fd, &byte, 1) != 1)
        _exit(90);
}
static void wait_byte_or_eof(int fd)
{
    char byte;
    ssize_t n;
    do {
        n = read(fd, &byte, 1);
    } while (n < 0 && errno == EINTR);
    if (n < 0)
        _exit(91);
}
int main(int argc, char **argv)
{
    if (argc == 5 && strcmp(argv[1], "--worker") == 0) {
        role = 1;
        child_id = getpid();
        checkpoint(3); /* exec completed in the same child PID */
        if (strcmp(argv[2], "early") == 0)
            wait_byte_or_eof(atoi(argv[3]));
        send_byte(STDOUT_FILENO, 'C');
        close(atoi(argv[4]));
        _exit(7);
    }
    if (argc != 2 || (strcmp(argv[1], "early") != 0 && strcmp(argv[1], "late") != 0))
        return 2;
    int go[2], done[2];
    if (pipe(go) != 0 || pipe(done) != 0)
        return 3;
    fork_result = fork();
    if (fork_result < 0)
        return 4;
    if (fork_result == 0) {
        role = 1;
        child_id = getpid();
        close(go[1]);
        close(done[0]);
        checkpoint(2); /* child: fork returned zero, before exec */
        char read_fd[24], write_fd[24];
        snprintf(read_fd, sizeof read_fd, "%d", go[0]);
        snprintf(write_fd, sizeof write_fd, "%d", done[1]);
        execl("/proc/self/exe", "process-lifecycle", "--worker", argv[1], read_fd, write_fd, (char *)NULL);
        _exit(92);
    }
    child_id = fork_result;
    close(go[0]);
    close(done[1]);
    checkpoint(1); /* parent: fork returned actual child PID */
    if (strcmp(argv[1], "early") == 0) {
        send_byte(STDOUT_FILENO, 'P');
        send_byte(go[1], 'x');
    } else {
        wait_byte_or_eof(done[0]);
        siginfo_t info;
        if (waitid(P_PID, (id_t)child_id, &info, WEXITED | WNOWAIT) != 0)
            return 5;
        checkpoint(5); /* child is exited, deliberately not reaped */
        send_byte(STDOUT_FILENO, 'P');
    }
    checkpoint(4); /* about to call waitpid; not a claim of observed CPU sleep */
    wait_result = waitpid(child_id, &wait_status, 0);
    checkpoint(6); /* actual wait result and raw status */
    close(go[1]);
    close(done[0]);
    return wait_result == child_id && WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == 7 ? 0 : 6;
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    if (pid == 0)
    {
        printf("子进程开始执行\n");
        sleep(2);
        printf("子进程执行完毕\n");
        exit(42);
    }

    printf("父进程等待子进程 %ld\n", (long)pid);

    int status = 0;
    pid_t res;

    do 
    {
        res = waitpid(pid, &status, 0);
    } while (res == -1 && errno == EINTR);

    if (res == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("子进程正常退出，退出码：%d\n",
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("子进程被信号终止，信号编号：%d\n",
               WTERMSIG(status));
    }

    return 0;
}
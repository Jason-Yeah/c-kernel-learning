#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int global = 10;

int main(void)
{
    int local = 100;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        global = 20;
        local = 200;

        printf("子进程：global=%d，地址=%p；local=%d，地址=%p\n",
               global, (void *)&global,
               local, (void *)&local);
    } else {
        waitpid(pid, NULL, 0);

        printf("父进程：global=%d，地址=%p；local=%d，地址=%p\n",
               global, (void *)&global,
               local, (void *)&local);
    }

    return 0;
}
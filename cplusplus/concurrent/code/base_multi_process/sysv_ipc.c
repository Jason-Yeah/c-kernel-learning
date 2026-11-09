#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linux/glibc semctl 的 SETVAL 等命令要求调用方提供这个 union。
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int wait_child(pid_t child)
{
    int status;
    while (waitpid(child, &status, 0) < 0)
    {
        if (errno != EINTR)
            return -1;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

static int sem_change(int id, short delta)
{
    struct sembuf operation = {0, delta, SEM_UNDO};
    while (semop(id, &operation, 1) < 0)
    {
        if (errno != EINTR)
            return -1;
    }
    return 0;
}

static int increment(int id, int *counter)
{
    for (int i = 0; i < 1000; ++i)
    {
        if (sem_change(id, -1) < 0)
            return -1;
        ++*counter;
        if (sem_change(id, 1) < 0)
            return -1;
    }
    return 0;
}

static int sem_demo(void)
{
    int id = semget(IPC_PRIVATE, 1, 0600);
    if (id < 0)
    {
        perror("semget");
        return 1;
    }

    union semun initial = {.val = 1};

    if (semctl(id, 0, SETVAL, initial) < 0)
    {
        perror("semctl SETVAL");
        semctl(id, 0, IPC_RMID);
        return 1;
    }
    int *counter = mmap(NULL, sizeof *counter, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (counter == MAP_FAILED)
    {
        perror("mmap");
        semctl(id, 0, IPC_RMID);
        return 1;
    }
    *counter = 0;
    fflush(NULL);


    pid_t child = fork();
    if (child < 0)
    {
        perror("fork");
        munmap(counter, sizeof *counter);
        semctl(id, 0, IPC_RMID);
        return 1;
    }
    if (child == 0)
        _exit(increment(id, counter) == 0 ? 0 : 1);

    int parent_result = increment(id, counter);
    int child_result = wait_child(child);
    printf("semid=%d protected counter=%d (expected 2000)\n", id, *counter);
    int ok = parent_result == 0 && child_result == 0 && *counter == 2000;
    munmap(counter, sizeof *counter);
    if (semctl(id, 0, IPC_RMID) < 0)
    {
        perror("semctl IPC_RMID");
        return 1;
    }
    return ok ? 0 : 1;
}

static int shm_demo(void)
{
    int id = shmget(IPC_PRIVATE, 128, 0600);
    if (id < 0)
    {
        perror("shmget");
        return 1;
    }

    char *memory = shmat(id, NULL, 0);

    if (memory == (void *)-1)
    {
        perror("shmat");
        shmctl(id, IPC_RMID, NULL);
        return 1;
    }
    // 已附加后标记删除：继承映射仍可使用，最后 detach 时释放。
    if (shmctl(id, IPC_RMID, NULL) < 0)
    {
        perror("shmctl IPC_RMID");
        shmdt(memory);
        return 1;
    }
    fflush(NULL);

    pid_t child = fork();
    if (child < 0)
    {
        perror("fork");
        shmdt(memory);
        return 1;
    }
    if (child == 0)
    {
        const char text[] = "hello from System V shared memory";
        memcpy(memory, text, sizeof text);
        shmdt(memory);
        _exit(0);
    }
    int result = wait_child(child); // 本演示在子进程退出后读取，不并发读写。
    if (result == 0)
        printf("shmid=%d parent read: %s\n", id, memory);
    shmdt(memory);
    return result == 0 ? 0 : 1;
}

struct Message
{
    long type; // 必须为正；发送/接收长度不包含它。
    char text[128];
};

static int msg_demo(void)
{
    int id = msgget(IPC_PRIVATE, 0600);
    if (id < 0)
    {
        perror("msgget");
        return 1;
    }
    fflush(NULL);

    
    pid_t child = fork();
    if (child < 0)
    {
        perror("fork");
        msgctl(id, IPC_RMID, NULL);
        return 1;
    }
    if (child == 0)
    {
        struct Message message = {7, "hello from message queue"};
        int rc;
        do
        {
            rc = msgsnd(id, &message, strlen(message.text), IPC_NOWAIT);
        } while (rc < 0 && errno == EINTR);
        _exit(rc == 0 ? 0 : 1);
    }
    // 只有一条小消息，先确认发送成功，再用 NOWAIT 收取，避免失败时永久等。
    int ok = wait_child(child) == 0;
    if (ok)
    {
        struct Message message;
        ssize_t length = msgrcv(id, &message, sizeof message.text, 7, IPC_NOWAIT);
        if (length < 0)
        {
            perror("msgrcv");
            ok = 0;
        }
        else
        {
            printf("msqid=%d type=%ld payload_bytes=%zd data=", id, message.type, length);
            fwrite(message.text, 1, (size_t)length, stdout);
            putchar('\n');
        }
    }
    if (msgctl(id, IPC_RMID, NULL) < 0)
    {
        perror("msgctl IPC_RMID");
        return 1;
    }
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: sysv_ipc sem|shm|msg\n");
        return 2;
    }
    if (strcmp(argv[1], "sem") == 0)
        return sem_demo();
    if (strcmp(argv[1], "shm") == 0)
        return shm_demo();
    if (strcmp(argv[1], "msg") == 0)
        return msg_demo();
    fprintf(stderr, "unknown mode\n");
    return 2;
}

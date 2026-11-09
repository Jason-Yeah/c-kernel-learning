#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

extern char **environ;

static int report(int error, const char *what)
{
    if (error != 0)
        fprintf(stderr, "%s: %s\n", what, strerror(error));
    return error;
}

struct Tokens
{
    char input[64];
    int count;
};

static void *tokenize(void *arg)
{
    struct Tokens *tokens = arg;
    char *position = NULL;
    for (char *token = strtok_r(tokens->input, ",", &position);
         token != NULL; token = strtok_r(NULL, ",", &position))
        ++tokens->count;
    return tokens;
}

static int reentrant_demo(void)
{
    struct Tokens data[2] = {{"one,two,three", 0}, {"a,b,c,d", 0}};
    pthread_t threads[2];
    int created = 0;
    for (; created < 2; ++created)
    {
        if (report(pthread_create(&threads[created], NULL, tokenize, &data[created]), "pthread_create"))
            break;
    }
    int ok = created == 2;
    for (int i = 0; i < created; ++i)
    {
        if (report(pthread_join(threads[i], NULL), "pthread_join"))
            ok = 0;
    }
    printf("strtok_r independent token counts=%d,%d\n", data[0].count, data[1].count);
    return ok && data[0].count == 3 && data[1].count == 4 ? 0 : 1;
}

struct SignalResult
{
    sigset_t set;
    int signal;
    int error;
};

static void *signal_receiver(void *arg)
{
    struct SignalResult *data = arg;
    data->error = sigwait(&data->set, &data->signal);
    return data;
}

static int signal_demo(void)
{
    struct SignalResult data;
    sigemptyset(&data.set);
    sigaddset(&data.set, SIGUSR1);
    data.signal = 0;
    data.error = 0;
    sigset_t original;
    if (report(pthread_sigmask(SIG_BLOCK, &data.set, &original), "pthread_sigmask"))
        return 1;
    pthread_t thread;
    if (report(pthread_create(&thread, NULL, signal_receiver, &data), "pthread_create"))
    {
        pthread_sigmask(SIG_SETMASK, &original, NULL);
        return 1;
    }
    int error = pthread_kill(thread, SIGUSR1); // 只发给本程序自己创建的线程。
    if (error != 0)
        pthread_cancel(thread);
    int joined = pthread_join(thread, NULL);
    pthread_sigmask(SIG_SETMASK, &original, NULL);
    printf("sigwait received SIGUSR1=%d\n", data.signal == SIGUSR1);
    return report(error, "pthread_kill") == 0 && report(joined, "pthread_join") == 0 && data.error == 0 && data.signal == SIGUSR1 ? 0 : 1;
}

static void *parked_worker(void *arg)
{
    sem_t *gate = arg;
    while (sem_wait(gate) < 0)
    {
        if (errno != EINTR)
            return NULL;
    }
    return gate;
}

static int fork_exec_demo(void)
{
    char program[] = "/bin/echo";
    char argument[] = "execve from multithreaded parent";
    char *arguments[] = {program, argument, NULL};
    sem_t gate;
    if (sem_init(&gate, 0, 0) < 0)
    {
        perror("sem_init");
        return 1;
    }
    pthread_t thread;
    if (report(pthread_create(&thread, NULL, parked_worker, &gate), "pthread_create"))
    {
        sem_destroy(&gate);
        return 1;
    }
    fflush(NULL);
    pid_t child = fork();
    int ok = 1;
    if (child == 0)
    {
        // fork 后不调用 stdio/分配/锁：直接 execve，失败只 _exit。
        execve(program, arguments, environ);
        _exit(127);
    }
    if (child < 0)
    {
        perror("fork");
        ok = 0;
    }
    else
    {
        int status;
        pid_t reaped;
        do
        {
            reaped = waitpid(child, &status, 0);
        } while (reaped < 0 && errno == EINTR);
        ok = reaped == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    sem_post(&gate);
    if (report(pthread_join(thread, NULL), "pthread_join"))
        ok = 0;
    sem_destroy(&gate);
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: pthread_environment reentrant|signal|fork-exec\n");
        return 2;
    }
    if (strcmp(argv[1], "reentrant") == 0)
        return reentrant_demo();
    if (strcmp(argv[1], "signal") == 0)
        return signal_demo();
    if (strcmp(argv[1], "fork-exec") == 0)
        return fork_exec_demo();
    fprintf(stderr, "unknown mode\n");
    return 2;
}

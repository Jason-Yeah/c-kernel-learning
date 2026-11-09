#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int report(int error, const char *what)
{
    if (error != 0)
        fprintf(stderr, "%s: %s\n", what, strerror(error));
    return error;
}

struct Argument
{
    int input;
    int output;
};

static void *calculate(void *arg)
{
    struct Argument *data = arg;
    data->output = data->input * 2;
    return data; // 返回指向 main 仍存活对象的指针，不能返回线程栈上局部对象地址。
}

static int lifecycle(void)
{
    pthread_attr_t attr;
    if (report(pthread_attr_init(&attr), "attr_init"))
        return 1;
    size_t size;
    int error = pthread_attr_getstacksize(&attr, &size);
    if (error == 0)
        printf("default stack size=%zu bytes (system-dependent)\n", size);
    pthread_t thread;
    struct Argument data = {21, 0};
    error = pthread_create(&thread, &attr, calculate, &data);
    pthread_attr_destroy(&attr); // 创建后 attr 不再是该线程正在使用的栈对象。
    if (report(error, "pthread_create"))
        return 1;
    void *result = NULL;
    if (report(pthread_join(thread, &result), "pthread_join"))
        return 1;
    printf("joined: output=%d returned_same_argument=%d\n", data.output, result == &data);
    return data.output == 42 && result == &data ? 0 : 1;
}

struct Counter
{
    pthread_mutex_t mutex;
    int value;
};

static void *increase(void *arg)
{
    struct Counter *counter = arg;
    for (int i = 0; i < 10000; ++i)
    {
        if (pthread_mutex_lock(&counter->mutex) != 0)
            return NULL;
        ++counter->value;
        if (pthread_mutex_unlock(&counter->mutex) != 0)
            return NULL;
    }
    return counter;
}

static int mutex_counter(void)
{
    struct Counter counter = {PTHREAD_MUTEX_INITIALIZER, 0};
    pthread_t threads[2];
    int created = 0;
    for (; created < 2; ++created)
    {
        int error = pthread_create(&threads[created], NULL, increase, &counter);
        if (report(error, "pthread_create"))
            break;
    }
    int ok = created == 2;
    for (int i = 0; i < created; ++i)
    {
        void *result = NULL;
        if (report(pthread_join(threads[i], &result), "pthread_join") || result != &counter)
            ok = 0;
    }
    printf("mutex protected counter=%d (expected 20000)\n", counter.value);
    pthread_mutex_destroy(&counter.mutex);
    return ok && counter.value == 20000 ? 0 : 1;
}

struct Condition
{
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    int ready;
    int value;
};

static void *condition_reader(void *arg)
{
    struct Condition *data = arg;
    if (pthread_mutex_lock(&data->mutex) != 0)
        return NULL;
    while (!data->ready) // 即使被唤醒，也要重新验证共享谓词。
    {
        int error = pthread_cond_wait(&data->changed, &data->mutex);
        if (error != 0)
        {
            pthread_mutex_unlock(&data->mutex);
            return NULL;
        }
    }
    int value = data->value;
    pthread_mutex_unlock(&data->mutex);
    printf("condition reader received=%d\n", value);
    return data;
}

static int condition_demo(void)
{
    struct Condition data = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0};
    pthread_t thread;
    if (report(pthread_create(&thread, NULL, condition_reader, &data), "pthread_create"))
    {
        pthread_cond_destroy(&data.changed);
        pthread_mutex_destroy(&data.mutex);
        return 1;
    }
    pthread_mutex_lock(&data.mutex);
    data.value = 42;
    data.ready = 1;
    pthread_cond_signal(&data.changed);
    pthread_mutex_unlock(&data.mutex);
    void *result = NULL;
    int error = pthread_join(thread, &result);
    pthread_cond_destroy(&data.changed);
    pthread_mutex_destroy(&data.mutex);
    return report(error, "pthread_join") == 0 && result == &data ? 0 : 1;
}

struct Slot
{
    sem_t empty;
    sem_t full;
    int value;
};

static int semaphore_wait(sem_t *sem)
{
    int rc;
    do
    {
        rc = sem_wait(sem);
    } while (rc < 0 && errno == EINTR);
    return rc;
}

static void *producer(void *arg)
{
    struct Slot *slot = arg;
    for (int i = 1; i <= 100; ++i)
    {
        if (semaphore_wait(&slot->empty) < 0)
            return NULL;
        slot->value = i;
        if (sem_post(&slot->full) < 0)
            return NULL;
    }
    return slot;
}

static int semaphore_demo(void)
{
    struct Slot slot;
    if (sem_init(&slot.empty, 0, 1) < 0)
    {
        perror("sem_init empty");
        return 1;
    }
    if (sem_init(&slot.full, 0, 0) < 0)
    {
        perror("sem_init full");
        sem_destroy(&slot.empty);
        return 1;
    }
    pthread_t thread;
    if (report(pthread_create(&thread, NULL, producer, &slot), "pthread_create"))
    {
        sem_destroy(&slot.full);
        sem_destroy(&slot.empty);
        return 1;
    }
    int sum = 0;
    int ok = 1;
    for (int i = 0; i < 100; ++i)
    {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 5;
        int rc;
        do
        {
            rc = sem_timedwait(&slot.full, &deadline);
        } while (rc < 0 && errno == EINTR);
        if (rc < 0)
        {
            perror("sem_timedwait");
            ok = 0;
            pthread_cancel(thread); // 错误时唤醒/取消可能等待 empty 的生产者。
            break;
        }
        sum += slot.value;
        sem_post(&slot.empty);
    }
    void *result = NULL;
    int error = pthread_join(thread, &result);
    printf("semaphore handoff sum=%d (expected 5050)\n", sum);
    sem_destroy(&slot.full);
    sem_destroy(&slot.empty);
    return ok && report(error, "pthread_join") == 0 && result == &slot && sum == 5050 ? 0 : 1;
}

struct CancelContext
{
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    int started;
    int finish;
};

static void unlock_cleanup(void *arg)
{
    pthread_mutex_unlock(arg);
}

static void *cancel_waiter(void *arg)
{
    struct CancelContext *data = arg;
    pthread_mutex_lock(&data->mutex);
    pthread_cleanup_push(unlock_cleanup, &data->mutex);
    data->started = 1;
    pthread_cond_signal(&data->changed);
    while (!data->finish)
        pthread_cond_wait(&data->changed, &data->mutex);
    pthread_cleanup_pop(1); // 若正常结束，也执行解锁；默认取消为 deferred。
    return data;
}

static int cancel_demo(void)
{
    struct CancelContext data = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0};
    pthread_t thread;
    if (report(pthread_create(&thread, NULL, cancel_waiter, &data), "pthread_create"))
    {
        pthread_cond_destroy(&data.changed);
        pthread_mutex_destroy(&data.mutex);
        return 1;
    }
    pthread_mutex_lock(&data.mutex);
    while (!data.started)
        pthread_cond_wait(&data.changed, &data.mutex);
    int error = pthread_cancel(thread);
    if (error != 0)
    {
        data.finish = 1;
        pthread_cond_signal(&data.changed);
    }
    pthread_mutex_unlock(&data.mutex);
    void *result = NULL;
    int joined = pthread_join(thread, &result);
    printf("cancelled=%d (cleanup handler unlocks the mutex)\n", result == PTHREAD_CANCELED);
    pthread_cond_destroy(&data.changed);
    pthread_mutex_destroy(&data.mutex);
    return report(error, "pthread_cancel") == 0 && report(joined, "pthread_join") == 0 && result == PTHREAD_CANCELED ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: pthread_basics lifecycle|mutex|condition|semaphore|cancel\n");
        return 2;
    }
    if (strcmp(argv[1], "lifecycle") == 0)
        return lifecycle();
    if (strcmp(argv[1], "mutex") == 0)
        return mutex_counter();
    if (strcmp(argv[1], "condition") == 0)
        return condition_demo();
    if (strcmp(argv[1], "semaphore") == 0)
        return semaphore_demo();
    if (strcmp(argv[1], "cancel") == 0)
        return cancel_demo();
    fprintf(stderr, "unknown mode\n");
    return 2;
}

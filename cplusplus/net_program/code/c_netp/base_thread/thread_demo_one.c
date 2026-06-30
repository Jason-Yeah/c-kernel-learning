#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <memory>

void* thread_main(void *arg);

int main(int argc, char* argv[])
{
    pthread_t t_id;
    int thread_param = 5;

    if(pthread_create(&t_id, NULL, thread_main, (void*)&thread_param)!=0)
    {
        puts("Error");
        return -1;
    }
    sleep(10);
    puts("end");
    return 0;
}

void* thread_main(void *arg)
{
    size_t i;
    // int cnt = *((int*) arg);
    for (i = 0; i < *arg; ++ i)
    {
        sleep(1);
        puts("running");
    }
    return NULL;
}

#include <cstdio>
#include <pthread.h>

#include <iostream>
#include <thread>

void *worker(void *arg)
{
    int id = *static_cast<int *>(arg);
    printf("worker %d\n", id);
    return nullptr;
}

void worker1(int id) { std::cout << "worker " << id << '\n'; }

int main()
{
    pthread_t tid;
    int id = 2;

    pthread_create(&tid, nullptr, worker, &id);
    pthread_join(tid, nullptr);

    std::thread t(worker1, 1);

    t.join();

    std::jthread t2(worker1, 22);
}
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUF_SIZE 5  // 缓冲区大小

int buffer[BUF_SIZE]; // 循环队列缓冲区
int front = 0;        // 消费者取数据的位置
int rear = 0;         // 生产者放数据的位置

sem_t mutex, slots, items;

// 生产者线程
void *producer(void *arg) {
    for (int i = 1; i <= 10; i++) { // 生产10个物品
        sem_wait(&slots);  // 1. 等待空位 (slots--)
        sem_wait(&mutex);  // 2. 加锁进入缓冲区

        buffer[rear] = i;
        printf("生产者: 放入物品 %d (位置 %d)\n", i, rear);
        rear = (rear + 1) % BUF_SIZE;

        sem_post(&mutex);  // 3. 解锁
        sem_post(&items);  // 4. 通知消费者：有新货了 (items++)
        usleep(100000);    // 模拟生产耗时
    }
    return NULL;
}

// 消费者线程
void *consumer(void *arg) {
    for (int i = 1; i <= 10; i++) {
        sem_wait(&items);  // 1. 等待物品 (items--)
        sem_wait(&mutex);  // 2. 加锁进入缓冲区

        int item = buffer[front];
        printf("消费者: 取出物品 %d (位置 %d)\n", item, front);
        front = (front + 1) % BUF_SIZE;

        sem_post(&mutex);  // 3. 解锁
        sem_post(&slots);  // 4. 通知生产者：有空位了 (slots++)
        usleep(300000);    // 模拟消费耗时（比生产慢）
    }
    return NULL;
}

int main() {
    pthread_t tid_p, tid_c;

    // 初始化信号量
    sem_init(&mutex, 0, 1);        // 互斥锁初值为 1
    sem_init(&slots, 0, BUF_SIZE); // 空位初值为 5
    sem_init(&items, 0, 0);        // 物品初值为 0

    pthread_create(&tid_p, NULL, producer, NULL);
    pthread_create(&tid_c, NULL, consumer, NULL);

    pthread_join(tid_p, NULL);
    pthread_join(tid_c, NULL);

    sem_destroy(&mutex);
    sem_destroy(&slots);
    sem_destroy(&items);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 定义任务函数类型：参数为 void*，无返回值
typedef void (*TaskFunction)(void*);

// 定义任务节点
typedef struct Task {
    TaskFunction func;
    void* arg;
    struct Task* next;
} Task;

// 定义任务队列，并添加线程同步所需的互斥量和条件变量，以及停止标志
typedef struct {
    Task* head;
    Task* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int stop;  // 停止标志，1 表示退出工作线程
} TaskQueue;

// 初始化任务队列及同步原语
void initQueue(TaskQueue* queue) {
    queue->head = queue->tail = NULL;
    queue->stop = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

// 销毁队列时释放同步资源
void destroyQueue(TaskQueue* queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
}

// 线程安全的入队操作
void enqueue(TaskQueue* queue, TaskFunction func, void* arg) {
    Task* newTask = malloc(sizeof(Task));
    if (!newTask) {
        perror("内存分配失败");
        exit(1);
    }
    newTask->func = func;
    newTask->arg = arg;
    newTask->next = NULL;
    
    pthread_mutex_lock(&queue->mutex);
    if (queue->tail) {
        queue->tail->next = newTask;
        queue->tail = newTask;
    } else {
        queue->head = queue->tail = newTask;
    }
    // 入队后通知等待的线程有新任务
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// 线程安全的出队操作，从队列头取出任务
Task* dequeue(TaskQueue* queue) {
    Task* task = queue->head;
    if (queue->head) {
        queue->head = queue->head->next;
        if (queue->head == NULL) {
            queue->tail = NULL;
        }
    }
    return task;
}

// 工作线程函数：不断等待并执行队列中的任务
void* worker_thread(void* arg) {
    TaskQueue* queue = (TaskQueue*) arg;
    
    while (1) {
        pthread_mutex_lock(&queue->mutex);
        // 当队列为空且未接收到停止信号时等待
        while (queue->head == NULL && !queue->stop) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        // 如果收到停止信号且队列为空，则退出循环
        if (queue->stop && queue->head == NULL) {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
        // 取出任务
        Task* task = dequeue(queue);
        pthread_mutex_unlock(&queue->mutex);
        
        if (task) {
            // 执行任务函数，并传入参数
            task->func(task->arg);
            free(task);
        }
    }
    return NULL;
}

// 示例任务函数，输出传入的整数值
void sampleTask(void* arg) {
    int* data = (int*) arg;
    printf("任务执行，值为: %d\n", *data);
    free(data);
}

int main(void) {
    TaskQueue queue;
    initQueue(&queue);
    
    pthread_t worker;
    // 创建工作线程
    if (pthread_create(&worker, NULL, worker_thread, (void*)&queue) != 0) {
        perror("创建线程失败");
        exit(1);
    }
    
    // 模拟入队多个任务
    for (int i = 0; i < 10; i++) {
        int* arg = malloc(sizeof(int));
        if (!arg) {
            perror("内存分配失败");
            exit(1);
        }
        *arg = i;
        enqueue(&queue, sampleTask, arg);
        usleep(100000);  // 模拟任务产生的延时
    }
    
    // 等待一段时间，确保任务被处理
    sleep(2);
    
    // 通知工作线程退出
    pthread_mutex_lock(&queue.mutex);
    queue.stop = 1;
    pthread_cond_signal(&queue.cond);
    pthread_mutex_unlock(&queue.mutex);
    
    // 等待工作线程结束
    pthread_join(worker, NULL);
    
    // 清理同步资源
    destroyQueue(&queue);
    
    return 0;
}

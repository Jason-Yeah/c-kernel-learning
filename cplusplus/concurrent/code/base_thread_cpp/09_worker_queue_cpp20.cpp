#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <thread>

int main()
{
    std::mutex mutex;
    std::condition_variable_any changed;
    std::deque<int> jobs;
    bool closed = false;
    int total = 0; // 只由 worker 写，main 在 join 后读取。

    std::jthread worker([&](std::stop_token stop)
    {
        for (;;)
        {
            std::unique_lock lock(mutex);
            bool available = changed.wait(lock, stop, [&]
            {
                return closed || !jobs.empty();
            });
            if (!available || jobs.empty())
                return;

            int job = jobs.front();
            jobs.pop_front();
            lock.unlock(); // 业务计算放到锁外。
            total += job;
        }
    });

    for (int value = 1; value <= 5; ++value)
    {
        {
            std::lock_guard lock(mutex);
            jobs.push_back(value);
        }
        changed.notify_one();
    }
    {
        std::lock_guard lock(mutex);
        closed = true;
    }
    changed.notify_all();
    worker.join(); // 正常路径：队列排空后返回，不提前 request_stop。

    std::cout << "drained total=" << total << '\n';
}

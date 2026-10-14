#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <thread>

int main()
{
    std::mutex mutex;
    std::condition_variable_any changed;
    bool stopped = false;

    std::jthread worker([&](std::stop_token stop)
    {
        std::unique_lock lock(mutex);
        changed.wait(lock, stop, [] { return false; });
        stopped = stop.stop_requested();
    });

    worker.request_stop(); // 发出请求；不是强制终止。
    worker.join();
    std::cout << "stop observed=" << stopped << '\n';
}

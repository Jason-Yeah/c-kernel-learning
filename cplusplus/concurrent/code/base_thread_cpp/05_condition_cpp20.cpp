#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

int main()
{
    std::mutex mutex;
    std::condition_variable changed;
    bool ready = false;
    int value = 0;
    int result = 0;

    std::jthread reader([&]
    {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return ready; });
        result = value; // 返回 wait 时仍持有锁。
    });

    {
        std::lock_guard lock(mutex);
        value = 42;
        ready = true;
    }
    changed.notify_one();
    reader.join();

    std::cout << "received=" << result << '\n';
}

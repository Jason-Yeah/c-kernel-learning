#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>

int main()
{
    std::shared_mutex mutex; // C++17 就已经提供。
    int value = 0;

    std::jthread writer([&]
    {
        for (int i = 0; i < 1000; ++i)
        {
            std::unique_lock lock(mutex);
            ++value;
        }
    });

    int snapshot = 0;
    for (int i = 0; i < 1000; ++i)
    {
        std::shared_lock lock(mutex);
        snapshot = value; // 共享锁只读取受保护数据。
    }
    writer.join();

    std::cout << "final=" << value << " snapshot=" << snapshot << '\n';
}

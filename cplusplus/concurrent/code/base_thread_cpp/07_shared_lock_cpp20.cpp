#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>

int main()
{
    // 读取时可以共享锁；修改时必须独占锁
    std::shared_mutex mutex; // C++17 就已经提供。
    int value = 0;

    std::jthread writer(
        [&]
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
        std::shared_lock lock(mutex); // 获得共享锁；若被写线程占用，就等待
        snapshot = value;             // 共享锁只读取受保护数据。
    }
    writer.join();

    std::cout << "final=" << value << " snapshot=" << snapshot << '\n';
}

#include <iostream>
#include <mutex>
#include <thread>

int main()
{
    std::mutex mutex;
    int counter = 0;

    auto increment = [&]
    {
        for (int i = 0; i < 10000; ++i)
        {
            std::lock_guard lock(mutex);
            ++counter;
        } // 每一轮离开作用域时自动解锁。
    };

    std::jthread a(increment);
    std::jthread b(increment);
    a.join();
    b.join();

    std::cout << "counter=" << counter << '\n';
}

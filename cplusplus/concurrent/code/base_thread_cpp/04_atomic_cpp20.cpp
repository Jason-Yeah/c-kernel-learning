#include <atomic>
#include <iostream>
#include <thread>

int main()
{
    std::atomic<int> counter{0};
    auto increment = [&]
    {
        for (int i = 0; i < 10000; ++i)
            counter.fetch_add(1, std::memory_order_relaxed);
    };

    std::jthread a(increment);
    std::jthread b(increment);
    a.join();
    b.join();

    // 这里只统计一个独立整数；其他数据的发布不能照搬 relaxed。
    std::cout << "atomic counter=" << counter.load() << '\n';
}

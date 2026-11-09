#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    std::atomic<int> cntr{0};

    std::vector<std::jthread> thds;

    for (int i = 0; i < 4; ++i)
    {
        thds.emplace_back( // & == this
            [i, &cntr]
            {
                for (int j = 0; j < 1000; j++)
                {
                    // 原子 + i
                    cntr.fetch_add(i, std::memory_order_relaxed);
                    // memory_order_relaxed
                    // 内存序吗，只要求当前原子的r/w原子性，不额外
                    // 建立线程之间的同步顺序
                }
            });
    }

    thds.clear(); // 析构 + join

    std::cout << cntr.load() << '\n';
}

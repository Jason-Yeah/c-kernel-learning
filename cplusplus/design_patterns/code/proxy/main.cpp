#include "proxy.hpp"
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    auto proxy =
        std::make_unique<SafeCounterProxy>(std::make_unique<UnsafeCounter>());
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [&proxy]
            {
                for (int j = 0; j < 100; j++)
                    proxy->inc();
            });
    }

    for (auto &t : threads)
        t.join();

    std::cout << "最终计数: " << proxy->getValue() << std::endl;

    return 0;
}

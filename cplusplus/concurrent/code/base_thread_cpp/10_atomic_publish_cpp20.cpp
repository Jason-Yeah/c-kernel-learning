#include <atomic>
#include <iostream>
#include <thread>

int main()
{
    int payload = 0;
    int observed = 0;
    std::atomic<bool> ready{false};

    std::jthread reader(
        [&]
        {
            // 等待值不再是 false，并接收发布的数据
            ready.wait(false, std::memory_order_acquire);
            observed = payload;
        });

    payload = 42;
    ready.store(true, std::memory_order_release);
    ready.notify_one();
    reader.join();

    std::cout << "read: " << observed << '\n';
}

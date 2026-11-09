#include <iostream>
#include <mutex>
#include <thread>

struct Account
{
    std::mutex mutex;
    int balance = 1000;
};

void transfer(Account &from, Account &to)
{
    // 获得这两把互斥锁；两把都获得后，才能继续执行。离开作用域时，自动释放两把锁
    // scoped 指“作用域”：对象出了作用域，就自动解锁
    // 如果 lock_guard 写成这样，有可能会死锁
    std::scoped_lock lock(from.mutex, to.mutex); // C++17 的双锁设施。
    --from.balance;
    ++to.balance;
}

int main()
{
    Account a, b;
    std::jthread first(
        [&]
        {
            for (int i = 0; i < 1000; ++i)
                transfer(a, b);
        });
    std::jthread second(
        [&]
        {
            for (int i = 0; i < 100; ++i)
                transfer(b, a);
        });
    first.join();
    second.join();

    std::cout << "A=" << a.balance << " B=" << b.balance << '\n';
}

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
    std::scoped_lock lock(from.mutex, to.mutex); // C++17 的双锁设施。
    --from.balance;
    ++to.balance;
}

int main()
{
    Account a, b;
    std::jthread first([&]
    {
        for (int i = 0; i < 1000; ++i)
            transfer(a, b);
    });
    std::jthread second([&]
    {
        for (int i = 0; i < 1000; ++i)
            transfer(b, a);
    });
    first.join();
    second.join();

    std::cout << "A=" << a.balance << " B=" << b.balance << '\n';
}

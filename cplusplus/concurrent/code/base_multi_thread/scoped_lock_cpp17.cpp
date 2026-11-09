#include "thread_group.hpp"
#include <iostream>
#include <mutex>
#include <stdexcept>

struct Account
{
    std::mutex mutex;
    int balance = 1000;
};

static void transfer(Account &from, Account &to)
{
    // 同时锁定两个不同 mutex，避免两线程按相反顺序持有/等待。
    std::scoped_lock lock(from.mutex, to.mutex);
    --from.balance;
    ++to.balance;
}

int main()
try
{
    Account a, b;
    ThreadGroup workers;
    workers.start([&] { for (int i = 0; i < 1000; ++i) transfer(a, b); });
    workers.start([&] { for (int i = 0; i < 1000; ++i) transfer(b, a); });
    workers.join();
    if (a.balance != 1000 || b.balance != 1000)
        throw std::runtime_error("incorrect balances");
    std::cout << "deadlock avoided: A=" << a.balance << " B=" << b.balance << '\n';
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

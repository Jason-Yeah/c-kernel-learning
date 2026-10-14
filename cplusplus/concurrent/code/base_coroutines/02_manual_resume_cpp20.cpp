#include "task.hpp"
#include <iostream>
#include <string>

Task conversation(std::string name)
{
    int value = 10;
    std::cout << name << ": first, value=" << value << '\n';

    co_await std::suspend_always{}; // 保存现场；没有自动安排下一次恢复。

    ++value;
    std::cout << name << ": second, value=" << value << '\n';
    co_return;
}

int main()
try
{
    auto a = conversation("A");
    auto b = conversation("B");
    std::cout << "created: bodies have not run yet\n";

    // main 充当最小调度器，在同一个 OS 线程里交替恢复两个协程。
    while (!a.done() || !b.done())
    {
        a.resume();
        b.resume();
    }
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

#include "task.hpp" // 本仓库提供的 Task，内部已包含 <coroutine>
#include <iostream>

Task count_twice()
{
    int value = 10;
    std::cout << "task: " << value << '\n';
    co_await std::suspend_always{};

    ++value;
    std::cout << "task: " << value << '\n';
    co_await std::suspend_always{};

    ++value;
    std::cout << "task: " << value << '\n';
    co_return;
}

int main()
{
    auto task = count_twice();
    std::cout << "main: created\n";

    task.resume();
    std::cout << "main: between\n";

    task.resume();
    std::cout << "main: next\n";

    task.resume();
    std::cout << "main: done=" << task.done() << '\n';
}

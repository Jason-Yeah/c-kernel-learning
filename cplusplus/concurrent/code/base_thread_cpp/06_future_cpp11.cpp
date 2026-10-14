#include <future>
#include <iostream>
#include <stdexcept>

int main()
{
    auto result = std::async(std::launch::async, []
    {
        return 21 * 2;
    });
    std::cout << "future result=" << result.get() << '\n';

    auto failure = std::async(std::launch::async, []() -> int
    {
        throw std::runtime_error("task failed");
    });

    try
    {
        failure.get(); // 在线程任务里保存的异常，在调用方重抛。
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "caught: " << e.what() << '\n';
    }
}

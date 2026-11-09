#include <future>
#include <iostream>
#include <stdexcept>

/**
 * 取结果的凭证：
 * 任务可能还没算完，先拿着凭证，之后再来取结果
 */

int main()
{
    // std::launch::async 指定：在 `另一个线程` 中执行这个函数
    // std::future<int> result
    auto result = std::async(std::launch::async, [] { return 21 * 2; });

    /**
     *   result.get()：
     *
     *   如果任务还没完成，就等待。
     *   如果任务已经完成，就取出返回值 42
     */

    std::cout << "future result=" << result.get() << '\n';

    auto failure = std::async(std::launch::async, []() -> int
                              { throw std::runtime_error("task failed"); });

    try
    {
        failure.get(); // 在线程任务里保存的异常，在调用方重抛。
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "caught: " << e.what() << '\n';
    }
}

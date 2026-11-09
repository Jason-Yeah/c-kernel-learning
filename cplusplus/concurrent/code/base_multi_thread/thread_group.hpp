#pragma once
#include <thread>
#include <utility>
#include <vector>

// C++11 join 所有者；仅用于会自行结束的示例。调用者必须先解除 worker 的等待。
class ThreadGroup
{
    std::vector<std::thread> threads_;

public:
    ThreadGroup() = default;
    ThreadGroup(const ThreadGroup &) = delete;
    ThreadGroup &operator=(const ThreadGroup &) = delete;
    ~ThreadGroup() { join(); }

    template <typename Function>
    void start(Function &&function)
    {
        threads_.emplace_back(std::forward<Function>(function));
    }

    void join()
    {
        for (auto &thread : threads_)
        {
            if (thread.joinable())
                thread.join();
        }
    }
};

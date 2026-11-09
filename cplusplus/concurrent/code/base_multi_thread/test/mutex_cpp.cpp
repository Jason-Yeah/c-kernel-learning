#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <stop_token>
#include <syncstream>
#include <thread>

class TaskQueue
{
    std::mutex mtx_;
    std::condition_variable_any cv_;

    std::queue<int> tsks_;

public:
    void push(int tsk)
    {
        {
            std::lock_guard lock(mtx_);
            tsks_.push(tsk);
        }
        cv_.notify_one();
    }

    void worker(std::stop_token stop)
    {
        while (true)
        {
            int task;

            {
                std::unique_lock lock(mtx_);

                // third param: what condition can let `wait` keep exec..
                // IF RET TREU, NOT WAIT
                auto has_task =
                    cv_.wait(lock, stop, [&] { return !tsks_.empty(); });
                if (!has_task)
                    return;

                task = tsks_.front();
                tsks_.pop();
            }

            std::osyncstream(std::cout) << "Processing: " << task << '\n';
        }
    }
};

int main()
{
    TaskQueue tskqueue;

    {
        std::jthread t1([&tskqueue](std::stop_token st)
                        { tskqueue.worker(st); });

        std::jthread t2([&tskqueue](std::stop_token st)
                        { tskqueue.worker(st); });

        for (int i = 0; i < 10; ++i)
            tskqueue.push(i);
    }

    std::cout << "All workers finished\n";
}

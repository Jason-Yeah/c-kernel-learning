#include "thread_group.hpp"
#include "bounded_queue.hpp"
#include <array>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

static void counter_demo()
{
    int value = 0;
    std::mutex mutex;
    ThreadGroup workers;
    for (int i = 0; i < 4; ++i)
    {
        workers.start([&]
                      {
                          for (int n = 0; n < 10000; ++n)
                          {
                              std::lock_guard<std::mutex> lock(mutex);
                              ++value;
                          }
                      });
    }
    workers.join();
    if (value != 40000)
        throw std::runtime_error("incorrect counter");
    std::cout << "C++11 mutex counter=" << value << '\n';
}

static void queue_demo()
{
    BoundedQueue queue(4);
    std::array<int, 2> sums{{0, 0}};
    ThreadGroup consumers;
    try
    {
        for (std::size_t i = 0; i < sums.size(); ++i)
        {
            consumers.start([&, i]
                            {
                                int value;
                                while (queue.pop(value))
                                    sums[i] += value; // 各线程拥有不同元素。
                            });
        }
        for (int n = 1; n <= 100; ++n)
            queue.push(n);
    }
    catch (...)
    {
        queue.close(); // 异常展开前唤醒消费者，再由 ThreadGroup 析构 join。
        throw;
    }
    queue.close();
    consumers.join();
    int total = sums[0] + sums[1];
    if (total != 5050)
        throw std::runtime_error("queue lost values");
    std::cout << "bounded queue sum=" << total << " closed and drained\n";
}

static void exception_demo()
{
    std::exception_ptr error;
    ThreadGroup worker;
    worker.start([&]
                 {
                     try
                     {
                         throw std::runtime_error("worker failure");
                     }
                     catch (...)
                     {
                         error = std::current_exception();
                     }
                 });
    worker.join(); // error 在 join 后才被 main 读取。
    if (error)
    {
        try
        {
            std::rethrow_exception(error);
        }
        catch (const std::runtime_error &e)
        {
            std::cout << "main observed: " << e.what() << '\n';
        }
    }
}

int main(int argc, char **argv)
try
{
    if (argc != 2)
        throw std::runtime_error("usage: modern_cpp11 counter|queue|exception");
    std::string mode(argv[1]);
    if (mode == "counter")
        counter_demo();
    else if (mode == "queue")
        queue_demo();
    else if (mode == "exception")
        exception_demo();
    else
        throw std::runtime_error("unknown mode");
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

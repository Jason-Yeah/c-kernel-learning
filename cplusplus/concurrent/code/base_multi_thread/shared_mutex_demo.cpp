#include "thread_group.hpp"
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

#if __cplusplus >= 201703L
using SharedMutex = std::shared_mutex; // C++17
#else
using SharedMutex = std::shared_timed_mutex; // C++14
#endif

int main()
try
{
    SharedMutex mutex;
    int value = 0;
    int snapshot = 0;
    ThreadGroup workers;
    for (int i = 0; i < 2; ++i)
    {
        workers.start([&]
                      {
                          for (int n = 0; n < 1000; ++n)
                          {
                              std::unique_lock<SharedMutex> lock(mutex);
                              ++value;
                          }
                      });
    }
    workers.start([&]
                  {
                      for (int n = 0; n < 1000; ++n)
                      {
                          std::shared_lock<SharedMutex> lock(mutex);
                          snapshot = value;
                      }
                  });
    workers.join();
    if (value != 2000)
        throw std::runtime_error("incorrect shared counter");
    std::cout << "shared mutex value=" << value << " reader_snapshot=" << snapshot << '\n';
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

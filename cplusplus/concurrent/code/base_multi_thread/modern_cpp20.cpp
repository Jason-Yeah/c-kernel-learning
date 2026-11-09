#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <semaphore>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

static void stop_demo()
{
    std::mutex mutex;
    std::condition_variable_any changed;
    bool observed_stop = false;
    std::jthread worker([&](std::stop_token stop)
                        {
                            std::unique_lock lock(mutex);
                            changed.wait(lock, stop, [] { return false; });
                            observed_stop = stop.stop_requested();
                        });
    worker.request_stop(); // 即使比 worker 的 wait 更早，也不会丢失这个请求。
    worker.join();
    if (!observed_stop)
        throw std::runtime_error("stop not observed");
    std::cout << "jthread cooperative stop observed=1\n";
}

static void atomic_wait_demo()
{
    std::string message;
    std::atomic<int> state{0};
    std::string observed;
    std::jthread reader([&]
                        {
                            state.wait(0, std::memory_order_acquire);
                            observed = message;
                        });
    message = "published payload";
    state.store(1, std::memory_order_release); // 发布 message 的写入。
    state.notify_one();
    reader.join();
    if (observed != message)
        throw std::runtime_error("publication failed");
    std::cout << "atomic wait read: " << observed << '\n';
}

static void semaphore_demo()
{
    std::counting_semaphore<2> permits(2);
    std::atomic<int> active{0};
    std::atomic<int> peak{0};
    std::vector<std::jthread> workers;
    for (int i = 0; i < 4; ++i)
    {
        workers.emplace_back([&]
                             {
                                 permits.acquire();
                                 int current = active.fetch_add(1) + 1;
                                 int previous = peak.load();
                                 while (previous < current && !peak.compare_exchange_weak(previous, current)) {}
                                 std::this_thread::sleep_for(std::chrono::milliseconds(2));
                                 active.fetch_sub(1);
                                 permits.release();
                             });
    }
    for (auto &thread : workers)
        thread.join();
    if (peak.load() < 1 || peak.load() > 2 || active.load() != 0)
        throw std::runtime_error("permit accounting failed");
    std::cout << "semaphore peak=" << peak.load() << " (limit 2)\n";
}

static void barrier_demo()
{
    std::array<int, 2> values{0, 0};
    std::array<int, 2> sums{0, 0};
    std::barrier phase(2);
    std::jthread a([&]
                  {
                      values[0] = 20;
                      phase.arrive_and_wait();
                      sums[0] = values[0] + values[1];
                  });
    std::jthread b;
    try
    {
        b = std::jthread([&]
                         {
                             values[1] = 22;
                             phase.arrive_and_wait();
                             sums[1] = values[0] + values[1];
                         });
    }
    catch (...)
    {
        phase.arrive_and_drop(); // 若第二个线程创建失败，第一线程仍需被放行。
        throw;
    }
    a.join();
    b.join();
    if (sums[0] != 42 || sums[1] != 42)
        throw std::runtime_error("barrier publication failed");
    std::cout << "barrier phase sums=42,42\n";
}

int main(int argc, char **argv)
try
{
    if (argc != 2)
        throw std::runtime_error("usage: modern_cpp20 stop|atomic-wait|semaphore|barrier");
    std::string mode(argv[1]);
    if (mode == "stop")
        stop_demo();
    else if (mode == "atomic-wait")
        atomic_wait_demo();
    else if (mode == "semaphore")
        semaphore_demo();
    else if (mode == "barrier")
        barrier_demo();
    else
        throw std::runtime_error("unknown mode");
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

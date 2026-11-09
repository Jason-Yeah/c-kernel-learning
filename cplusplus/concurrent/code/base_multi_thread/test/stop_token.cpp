#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>

using namespace std::chrono_literals;

void worker(std::stop_token token, int id)
{
    while (!token.stop_requested())
    {
        std::cout << "worker " << id << " running\n";
        std::this_thread::sleep_for(200ms);
    }

    std::cout << "worker " << id << " stopping\n";
}

int main()
{
    std::jthread t(worker, 1);

    std::this_thread::sleep_for(1s);

    t.request_stop();
    t.join();

    std::cout << "main finished\n";
}

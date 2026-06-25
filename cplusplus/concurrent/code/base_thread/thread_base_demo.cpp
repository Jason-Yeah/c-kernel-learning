#include <chrono>
#include <iostream>
#include <thread>

void worker(int id)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "worker " << id << std::endl;
}

int main()
{
    std::thread t1(worker, 1);

    if (t1.joinable())
        t1.join();

    std::thread t2(worker, 2);
    t2.detach();

    fputs("1111\n", stdout);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}

#include <functional>
#include <iostream>
#include <thread>

void calculate(int input, int &output) { output = input * 2; }

int main()
{
    int result = 0;
    // 不能&result 这是指针类型ranh 
    std::thread worker(calculate, 21, std::ref(result));
    worker.join(); // 线程完成后才读取它写入的 result。

    std::cout << "result=" << result << '\n';
}

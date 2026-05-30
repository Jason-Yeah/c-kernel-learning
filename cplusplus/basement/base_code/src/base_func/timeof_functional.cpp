#include <functional>
#include <iostream>
#include <chrono>

void benchmark_overhead() {
    constexpr int N = 10'000'000;

    // ─── 直接调用 vs std::function 调用 ───
    auto lambda = [](int x) { return x * 2; };

    // 直接调用（编译期确定）
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile int sum1 = 0;
    for (int i = 0; i < N; ++i) sum1 = lambda(i);
    auto t2 = std::chrono::high_resolution_clock::now();

    // 通过 std::function 调用（运行时多态，间接调用）
    std::function<int(int)> func = lambda;
    auto t3 = std::chrono::high_resolution_clock::now();
    volatile int sum2 = 0;
    for (int i = 0; i < N; ++i) sum2 = func(i);
    auto t4 = std::chrono::high_resolution_clock::now();

    auto direct = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto wrapped = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    std::cout << "直接调用: " << direct << "ms\n";
    std::cout << "std::function: " << wrapped << "ms\n";
    std::cout << "开销倍数: " << (double)wrapped / direct << "x\n";
}

int main()
{
    benchmark_overhead();
    return 0;
}

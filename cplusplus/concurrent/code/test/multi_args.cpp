#include <cstdarg>
#include <cstdio>
#include <iostream>

int sum(int cnt, ...)
{
    va_list args;
    va_start(args, cnt);

    int res = 0;
    for (int i = 0; i < cnt; i++)
    {
        res += va_arg(args, int);
    }

    va_end(args);
    return res;
}

template <typename... Args> void show_cnt(Args... args)
{
    std::cout << "参数数量：" << sizeof...(args) << '\n';
}

template <typename... Args> void print(const Args &...args)
{
    ((std::cout << args << ' '), ...);
    std::cout << '\n';
}

template <typename... Args> auto sum1(Args... args) { return (0 + ... + args); }

int main()
{
    printf("%d\n", sum(3, 1, 2, 10));

    show_cnt(10, 20, 30, 40);

    print(10, 3.14, "hello"); // 输出：10 3.14 hello
    print("result =", 42);    // 输出：result = 42
    print();                  // 输出一个换行

    std::cout << sum1(10, 20, 30) << '\n'; // 60
    std::cout << sum1(1, 2.5, 3) << '\n';  // 6.5
    std::cout << sum1() << '\n';           //
}
#include <iostream>

int main() {
    // ─── 无捕获 lambda：等价于一个空结构体 ───
    auto lambda0 = []() { return 42; };
    std::cout << "无捕获 lambda 大小: " << sizeof(lambda0) << '\n';
    // 输出: 1  ← 空类，占 1 字节（C++ 要求每个对象有唯一地址）

    // ─── 按值捕获一个 int ───
    int x = 100;
    auto lambda1 = [x]() { return x; };
    std::cout << "捕获一个 int: " << sizeof(lambda1) << '\n';
    // 输出: 4  ← 内部存储了一个 int

    // ─── 按值捕获一个 double ───
    double y = 3.14;
    auto lambda2 = [y]() { return y; };
    std::cout << "捕获一个 double: " << sizeof(lambda2) << '\n';
    // 输出: 8

    // ─── 按值捕获多个变量 ───
    int a = 1; double b = 2.0; char c = 'A';
    auto lambda3 = [a, b, c]() { return a + b + c; };
    std::cout << "捕获 int+double+char: " << sizeof(lambda3) << '\n';
    // 输出: 24  ← 8(对齐double) + 8(double) + 4(int) + 1(char) + 3(padding)

    // ─── 按引用捕获：只存指针 ───
    auto lambda4 = [&x, &y]() { return x + y; };
    std::cout << "按引用捕获两个: " << sizeof(lambda4) << '\n';
    // 输出: 16  ← 两个指针，各 8 字节

    // ─── 混合捕获 ───
    auto lambda5 = [x, &y]() { return x + y; };
    std::cout << "混合捕获(值int+引用double): " << sizeof(lambda5) << '\n';
    // 输出: 16  ← 4(int) + 4(padding) + 8(指针)
    //            或者 8(指针) + 4(int) + 4(padding)（取决于成员顺序）

    return 0;
}

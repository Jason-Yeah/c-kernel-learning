#include <iostream>

// inline 允许此定义安全地放进头文件并被多个 .cpp 包含。
// 它不保证编译器一定把调用替换为乘法指令。
inline int square(int value) { return value * value; }

constexpr int cube(int value) { return value * value * value; } // constexpr 函数也隐含 inline

int main() {
    const int value = 4;
    std::cout << square(value) << ", " << cube(value) << '\n';

    auto functionAddress = &square; // inline 函数仍是可取地址的普通函数
    std::cout << functionAddress(5) << '\n';
}

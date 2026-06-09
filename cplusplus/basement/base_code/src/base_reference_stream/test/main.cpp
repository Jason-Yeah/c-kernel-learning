// main.cpp
#include <iostream>

// 一个简单的模板函数，用于演示
template <typename T>
void printValue(T&& value) {
    std::cout << "通用版本: " << value << std::endl;
}

// 一个针对左值引用的显式重载
void printValue(int& value) {
    std::cout << "左值引用版本: " << value << std::endl;
}

int main() {
    int a = 10;
    printValue(a);      // 调用1：传入左值
    printValue(20);     // 调用2：传入右值
    return 0;
}
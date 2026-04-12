#include <iostream>

template <typename T>
inline void callWithMaxValue(const T&a, const T&b)
{
    std::cout << "Max value: " << (a > b ? a : b) << std::endl;
}

int main()
{
    int a, b;
    std::cout << "Enter two integers: ";
    std::cin >> a >> b;

    callWithMaxValue(a, b);

    callWithMaxValue(-0.1, 7.9);
    
    return 0;
}
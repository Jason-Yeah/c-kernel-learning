#include <iostream>

int main()
{
    int a = 1;
    int &b = a;

    std::cout << "The address of a is : " << &a << std::endl;
    std::cout << "The address of b is : " << &b << std::endl;

    return 0;
}
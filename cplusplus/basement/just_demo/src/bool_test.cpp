#include <iostream>

int main()
{
    bool flag = true;

    std::cout << flag << std::endl; // only 1 or 0;

    std::cout << std::boolalpha;
    std::cout << flag << std::endl; // true or false;

    std::cout << sizeof(flag) << std::endl;

    return 0;
}
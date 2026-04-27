#include "../inc/const_test.h"

const int gval2 = 20;

void print_addr()
{
    std::cout << "Address of val: " << &val << std::endl;
    std::cout << "Address of gval2: " << &gval2 << std::endl;
}
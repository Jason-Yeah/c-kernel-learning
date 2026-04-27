#include "../inc/const_test.h"

int main()
{
    int a = 1;
    int &b = a;

    std::cout << "The address of a is : " << &a << std::endl;
    std::cout << "The address of b is : " << &b << std::endl;

    int c = 10;
    int *pi = nullptr;
    int *&pr = pi;
    pr = &c;

    // %p 输出地址
    printf("%p %p %p\n", &c, pi, pr);
    printf("%d %d %d\n", *(&c), *pi, *pr);

    *pr = 40;
    *pi = 50;
    printf("%d %d %d\n", *(&c), *pi, *pr);

    printf("==============================\n");
    print_addr();

    std::cout << "The address of val in main: " << &val << std::endl;
    std::cout << "The address of gval2 in main: " << &gval2 << std::endl;

    {
        const double PI = 3.14;
        const double* ptr = &PI;
    }

    {
        int err_num = 0;
        int* const p = &err_num;
        //p ++ ; // error: increment of read-only variable 'p'
        *p = 3; // ok
        printf("The value of p is : %p\n", p);
    }

    {
        int err = 0;
        const int *p = &err;
        // int* p2 = p;

        constexpr int ii = 20;
    }

    return 0;
}
#include <iostream>

// typedef int (*funcp)(int, int);

using funcp = int(*)(int, int);

int add(int a, int b)
{
    return a + b;
}

// ---

struct _adder
{
    int __to_add;
    _adder(int val) : __to_add(val) {}
    // Functors
    int operator() (int x)
    {
        return x + __to_add;
    }
}; 

int main()
{
    funcp func;
    func = &add;
    
    std::cout << func(1, 2) << std::endl;

    _adder adder(5);
    // Functors
    std::cout << adder(10) << std::endl;

    return 0;
}
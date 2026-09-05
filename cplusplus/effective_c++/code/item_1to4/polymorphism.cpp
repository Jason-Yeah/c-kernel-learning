#include <iostream>

class Base1 {
public:
    int a = 10;
};

class Base2 {
public:
    int b = 20;
};

class Derived : public Base1, public Base2 {
public:
    int c = 30;
};

int main()
{
    Derived d;

    Derived* pd = &d;
    Base1* p1 = &d;
    Base2* p2 = &d;

    std::cout << pd << '\n';
    std::cout << p1 << '\n';
    std::cout << p2 << '\n';
}

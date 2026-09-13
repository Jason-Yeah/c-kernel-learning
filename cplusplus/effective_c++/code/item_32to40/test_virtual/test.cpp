#include <iostream>

struct Base
{
    int x = 10;
};

struct Left : virtual public Base
{
    int l = 20;
};

struct Right : virtual public Base
{
    int r = 30;
};

struct Derived : public Left, public Right
{
    int d = 40;
};

int main()
{
    Derived obj;

    Left *pl = &obj;
    Right *pr = &obj;

    Base *pb1 = pl;
    Base *pb2 = pr;

    std::cout << &obj << '\n';
}
#include <iostream>

class Base
{
public:
    Base()
    {
        std::cout << "Base ctor\n";
        print(); // 构造期间调用 virtual
    }

    virtual ~Base()
    {
        std::cout << "Base dtor\n";
        print(); // 析构期间调用 virtual
    }

    virtual void print() { std::cout << "Base::print\n"; }
};

class Derived : public Base
{
public:
    Derived() { std::cout << "Derived ctor\n"; }

    ~Derived() override { std::cout << "Derived dtor\n"; }

    void print() override { std::cout << "Derived::print\n"; }
};

int main()
{
    Derived d;

    std::cout << "normal call\n";
    d.print();
}
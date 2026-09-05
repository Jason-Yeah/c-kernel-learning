#include <iostream>

class Trace
{
public:
    Trace()
    {
        std::cout << "Trace default ctor\n";
    }

    Trace(int x)
    {
        std::cout << "Trace(int) ctor, x=" << x << '\n';
    }

    Trace& operator=(int x)
    {
        std::cout << "Trace::operator=(int), x=" << x << '\n';
        return *this;
    }
};

class Bad
{
public:
    Bad(int x)
    {
        value_ = x;
    }

private:
    Trace value_;
};

class Good
{
public:
    Good(int x)
        : value_(x)
    {
    }

private:
    Trace value_;
};

int main()
{
    Bad a{10};
    Good b{20};
}

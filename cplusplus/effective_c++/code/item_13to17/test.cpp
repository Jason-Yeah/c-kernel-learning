#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

class Widget
{
public:
    Widget() { std::cout << "construct Widget, this = " << this << '\n'; }

    ~Widget() { std::cout << "destroy Widget, this = " << this << '\n'; }

    int value = 123456;
};

int priority(bool shouldThrow)
{
    if (shouldThrow)
        throw std::runtime_error("priority calculation failed");

    return 10;
}

void processWidget(const std::shared_ptr<Widget> &, int priorityValue)
{
    std::cout << "process priority " << priorityValue << '\n';
}

void leakDemo()
{
    Widget *raw = new Widget;

    std::cout << "raw = " << raw << '\n';

    // 此时 raw 还没有交给 shared_ptr
    int p = priority(true); // ← 在这里抛异常

    // 永远执行不到这里
    std::shared_ptr<Widget> owner(raw);

    processWidget(owner, p);
}

int main()
{
    try
    {
        leakDemo();
    }
    catch (const std::exception &error)
    {
        std::cout << "caught: " << error.what() << '\n';
    }
}
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

class Widget
{
public:
    Widget() { std::cout << "construct Widget\n"; }
    ~Widget() { std::cout << "destroy Widget\n"; }
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

int main()
{
    try
    {
        // 独立语句结束时，Widget 已由 shared_ptr 完整接管。
        auto widget = std::make_shared<Widget>();
        processWidget(widget,
                      priority(true)); // 抛异常后，widget 在栈展开时析构
    }
    catch (const std::exception &error)
    {
        std::cout << "caught: " << error.what() << '\n';
    }

    // C++17 前不要写：
    // processWidget(std::shared_ptr<Widget>(new Widget), priority(true));
}

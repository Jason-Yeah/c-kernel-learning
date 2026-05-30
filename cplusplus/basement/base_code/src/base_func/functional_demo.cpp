#include <iostream>
#include <functional>
#include <vector>
#include <memory>

int add(int a, int b) {
    return a + b;
}

struct multiplier
{
    int operator()(int a, int b) const 
    {
        return a * b;
    }
};

// Define a callback type using std::function
using Callback = std::function<void(int)>;

void triggerEvent(const Callback& cb, int value) {
    cb(value);
}

class Calculator
{
    public:
    int res;
    Calculator() : res(0) {}
    int subtract(int a, int b) 
    {
        res = a - b;
        return res;
    }
};

int main()
{
    std::function<int(int, int)> func;
    func = add;
    std::cout << "Addition: " << func(3, 4) << std::endl;

    func = multiplier{};
    std::cout << "Multiplication: " << func(3, 4) << std::endl;

    func = [](int a, int b) -> int {
        return a - b;
    };

    std::cout << "Subtraction: " << func(3, 4) << std::endl;

    std::vector<std::function<int(int, int)>> callbacks;
    callbacks.emplace_back(add);
    callbacks.emplace_back(multiplier{});
    callbacks.emplace_back([](int a, int b) -> int {
        return a - b;
    });

    for (const auto& callback: callbacks)
    {
        std::cout << callback(5, 2) << std::endl;
    }

    triggerEvent([](int x){ 
        std::cout << "Event triggered with value: " << x << std::endl; 
    }, 10);
    
    struct Printer
    {
        void operator()(int x) const 
        {
            std::cout << "Message: " << x << std::endl;
        }
    } printer;

    triggerEvent(printer, 798);

    auto new_add = std::bind(add, 10, std::placeholders::_1);
    std::cout << "New Addition: " << new_add(5) << std::endl;

    Calculator calc;
    auto new_sub = std::bind(&Calculator::subtract, &calc, std::placeholders::_1, std::placeholders::_2);
    std::cout << "New Subtraction: " << new_sub(10, 3) << std::endl;
    
    auto new_substract_lmb = [&calc](int a) {
        return calc.subtract(5, a);
    };

    new_substract_lmb(3);
    std::cout << "Subtraction result stored in Calculator: " << calc.res << std::endl;

    return 0;
}
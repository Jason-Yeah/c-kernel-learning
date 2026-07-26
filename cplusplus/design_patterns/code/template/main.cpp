#include "coffee_tea.hpp"
#include <iostream>
#include <memory>

int main()
{
    auto coffee = std::make_unique<Coffee>();
    coffee->prepare_recipe();

    auto tea = std::make_unique<Tea>();
    tea->prepare_recipe();

    return 0;
}
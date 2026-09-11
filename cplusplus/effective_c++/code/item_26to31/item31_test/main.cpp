// main.cpp
#include "person.hpp"

#include <iostream>

int main()
{
    auto person = Person::create("Jason", 20);

    std::cout << person->name() << '\n';
    std::cout << person->age() << '\n';
}
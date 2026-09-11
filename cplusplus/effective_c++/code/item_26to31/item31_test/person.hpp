#pragma once

#include <memory>
#include <string>

class Person
{
public:
    virtual ~Person() = default;

    virtual std::string name() const = 0;
    virtual int age() const = 0;

    static std::unique_ptr<Person> create(std::string name, int age);
};
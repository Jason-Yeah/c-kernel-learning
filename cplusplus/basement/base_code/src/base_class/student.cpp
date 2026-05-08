#include "student.h"

Stu::Stu(const std::string& name, int age)
    : name_(name), age_(age) {}

void Stu::set_name(const std::string& name)
{
    name_ = name;
}

std::string Stu::get_name() const
{
    return name_;
}

void Stu::set_age(int age)
{
    if (age > 0)
        age_ = age;
}

int Stu::get_age() const
{
    return age_;
}
#include "person.hpp"
#include <utility>

class RealPerson final : public Person
{
public:
    RealPerson(std::string name, int age) : name_(std::move(name)), age_(age) {}

    std::string name() const override { return name_; }

    int age() const override { return age_; }

private:
    std::string name_;
    int age_;
};

std::unique_ptr<Person> Person::create(std::string name, int age)
{
    return std::make_unique<RealPerson>(std::move(name), age);
}
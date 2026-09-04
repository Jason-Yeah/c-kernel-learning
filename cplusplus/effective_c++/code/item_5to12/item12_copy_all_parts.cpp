#include <iostream>
#include <string>
#include <utility>

class Base
{
public:
    explicit Base(std::string id) : id_(std::move(id)) {}

    Base(const Base &rhs) : id_(rhs.id_) {}

    Base &operator=(const Base &rhs)
    {
        id_ = rhs.id_;
        return *this;
    }

    const std::string &id() const { return id_; }

private:
    std::string id_;
};

class Derived : public Base
{
public:
    Derived(std::string id, int value) : Base(std::move(id)), value_(value) {}

    Derived(const Derived &rhs) : Base(rhs), value_(rhs.value_) {}

    Derived &operator=(const Derived &rhs)
    {
        Base::operator=(rhs);
        value_ = rhs.value_;
        return *this;
    }

    int value() const { return value_; }

private:
    int value_{};
};

int main()
{
    Derived source{"source-id", 42};
    auto copied = source;

    Derived assigned{"old-id", 0};

    std::cout << "assigned: " << assigned.id() << ", " << assigned.value()
              << '\n';
    assigned = source;

    std::cout << "copied: " << copied.id() << ", " << copied.value() << '\n';
    std::cout << "assigned: " << assigned.id() << ", " << assigned.value()
              << '\n';
}

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>

class Trace
{
public:
    explicit Trace(const char *name, bool throwInDestructor = false)
        : name_(name), throwInDestructor_(throwInDestructor)
    {
        std::cout << "construct " << name_ << '\n';
    }

    ~Trace() noexcept(false)
    {
        std::cout << "destroy " << name_ << '\n';

        if (throwInDestructor_)
        {
            std::cout << "destructor throws: " << name_ << '\n';
            throw std::runtime_error("exception from destructor");
        }
    }

private:
    const char *name_;
    bool throwInDestructor_;
};

void foo()
{
    // 栈展开时，a 的析构函数会再次 throw
    Trace a{"foo::a", true};

    std::cout << "foo: before first throw\n";

    throw std::runtime_error("first exception from foo");
}

void bar()
{
    Trace b{"bar::b"};

    foo();
}

int main()
{
    try
    {
        Trace m{"main::m"};

        bar();
    }
    catch (const std::exception &e)
    {
        // 理论上这次你会发现根本到不了这里
        std::cout << "caught: " << e.what() << '\n';
    }

    std::cout << "main end\n";
}
#include <iostream>
#include <memory>
#include <string>

class Trace
{
public:
    Trace() { std::cout << "construct Trace\n"; }
    ~Trace() { std::cout << "destroy Trace\n"; }
};

int main()
{
    Trace *one = new Trace;
    delete one;

    std::cout << "array begins\n";
    Trace *many = new Trace[3];
    delete[] many; // 会调用 3 次析构函数

    auto managedOne = std::make_unique<Trace>();
    auto managedMany = std::make_unique<Trace[]>(2); // unique_ptr<Trace[]>

    using AddressLines = std::string[4];
    std::string *address = new AddressLines;
    delete[] address; // AddressLines 的真实类型是数组

    // delete many; // 错误：new[] 得到的数组必须用 delete[]
}

#include <exception>
#include <iostream>
#include <stdexcept>

class NetworkLease
{
public:
    explicit NetworkLease(int id) : id_(id)
    {
        std::cout << "acquire lease " << id_ << '\n';
    }

    ~NetworkLease() noexcept { std::cout << "release lease " << id_ << '\n'; }

    // 不允许拷贝
    NetworkLease(const NetworkLease &) = delete;
    NetworkLease &operator=(const NetworkLease &) = delete;

private:
    int id_{};
};

void sendRequest()
{
    NetworkLease lease{42}; // 获取资源后立刻交给局部 RAII 对象
    std::cout << "request starts\n";
    throw std::runtime_error("network error");

    // 后面的语句都不执行

} // 栈展开时仍会调用 lease 的析构函数

int main()
{
    try
    {
        sendRequest();
    }
    catch (const std::exception &error)
    {
        std::cout << "caught: " << error.what() << '\n';
    }
}

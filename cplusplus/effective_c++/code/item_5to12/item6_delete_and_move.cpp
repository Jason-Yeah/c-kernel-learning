#include <iostream>
#include <string>
#include <utility>

class Connection
{
public:
    explicit Connection(std::string endpoint) : endpoint_(std::move(endpoint))
    {
    }

    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    Connection(Connection &&) noexcept = default;
    Connection &operator=(Connection &&) noexcept = default;

    const std::string &endpoint() const { return endpoint_; }

private:
    std::string endpoint_;
};

int main()
{
    Connection source{"db.example.internal"};
    Connection target = std::move(source); // 可以移动：转移对象状态

    std::cout << "target endpoint: " << target.endpoint() << '\n';

    // Connection copied = target; // 编译错误：复制被 = delete 明确拒绝
}

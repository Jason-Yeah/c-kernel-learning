#include <iostream>
#include <memory>
#include <utility>
#include <vector>

struct Connection
{
    explicit Connection(int id) : id(id) {}
    int id{};
};

class SharedConnection
{
public:
    explicit SharedConnection(int id)
        : connection_(std::make_shared<Connection>(id))
    {
    }

    long owners() const { return connection_.use_count(); }
    int id() const { return connection_->id; }

private:
    std::shared_ptr<Connection> connection_;
};

class UniqueConnection
{
public:
    explicit UniqueConnection(int id)
        : connection_(std::make_unique<Connection>(id))
    {
    }

    // 可移动不可拷贝
    UniqueConnection(const UniqueConnection &) = delete;
    UniqueConnection &operator=(const UniqueConnection &) = delete;
    UniqueConnection(UniqueConnection &&) noexcept = default;
    UniqueConnection &operator=(UniqueConnection &&) noexcept = default;

    int id() const { return connection_->id; }

private:
    std::unique_ptr<Connection> connection_;
};

int main()
{
    SharedConnection sharedA{1};
    SharedConnection sharedB = sharedA; // 两个包装对象共享同一个 Connection
    std::cout << "shared id=" << sharedB.id() << ", owners=" << sharedA.owners()
              << '\n';

    std::vector<int> dataA{1, 2, 3};
    std::vector<int> dataB = dataA; // 深拷贝元素
    dataB[0] = 9;
    std::cout << "deep copy: " << dataA[0] << " and " << dataB[0] << '\n';

    UniqueConnection uniqueA{2};
    UniqueConnection uniqueB = std::move(uniqueA); // 责任转移，不是复制
    std::cout << "moved unique id=" << uniqueB.id() << '\n';

    // UniqueConnection uniqueC = uniqueB; // 编译错误：唯一资源不能复制
}

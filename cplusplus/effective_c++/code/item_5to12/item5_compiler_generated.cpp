#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// 成员都是值语义类型：编译器生成的复制、移动、析构都是正确的。
struct Profile
{
    std::string name;
    std::vector<int> scores;
};

// unique_ptr 表达唯一所有权，因此该类型不可复制、但可移动。
struct UniqueLog
{
    std::unique_ptr<std::string> path;
};

int main()
{
    Profile ada{"Ada", {95, 100}};
    Profile copied = ada; // 编译器逐成员复制 string 和 vector
    copied.name = "Grace";
    copied.scores.push_back(88);

    std::cout << "original: " << ada.name << ", scores=" << ada.scores.size()
              << '\n';
    std::cout << "copied: " << copied.name
              << ", scores=" << copied.scores.size() << '\n';

    static_assert(!std::is_copy_constructible_v<UniqueLog>);
    static_assert(std::is_move_constructible_v<UniqueLog>);

    UniqueLog first{std::make_unique<std::string>("app.log")};
    UniqueLog second = std::move(first); // 转移 unique_ptr 的所有权
    std::cout << "moved path: " << *second.path << '\n';

    // UniqueLog third = second; // 编译错误：unique_ptr 不能复制

    return 0;
}

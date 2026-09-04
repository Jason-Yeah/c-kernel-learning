#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Profile {
public:
    Profile(std::string name, std::vector<int> scores)
        : name_(std::move(name)), scores_(std::move(scores)) {}

    void updateUnsafe(std::string name, std::vector<int> scores, bool simulateFailure) {
        name_ = std::move(name); // 旧状态已被改变
        if (simulateFailure) throw std::runtime_error("failure after changing name");
        scores_ = std::move(scores);
    }

    void updateStrong(std::string name, std::vector<int> scores) {
        Profile next{std::move(name), std::move(scores)}; // 这里失败时 *this 未变
        swap(*this, next);                               // 提交阶段只交换标准容器
    }

    friend void swap(Profile& left, Profile& right) noexcept {
        using std::swap;
        swap(left.name_, right.name_);
        swap(left.scores_, right.scores_);
    }

    void print() const {
        std::cout << name_ << ", scores=" << scores_.size() << '\n';
    }

private:
    std::string name_;
    std::vector<int> scores_;
};

int main() {
    Profile profile{"Ada", {90, 95}};
    try {
        profile.updateUnsafe("Grace", {100}, true);
    } catch (const std::exception& error) {
        std::cout << "unsafe failed: " << error.what() << '\n';
    }
    profile.print(); // name 已变为 Grace，scores 仍是旧数据：部分更新

    profile.updateStrong("Lin", {80, 88, 92});
    profile.print();
}

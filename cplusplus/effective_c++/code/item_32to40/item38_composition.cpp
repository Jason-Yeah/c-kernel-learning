#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

class IntSet {
public:
    bool contains(int value) const {
        return std::find(elements_.begin(), elements_.end(), value) != elements_.end();
    }
    bool insert(int value) {
        if (contains(value)) return false;
        elements_.push_back(value);
        return true;
    }
    std::size_t size() const { return elements_.size(); }
private:
    std::vector<int> elements_; // 借助 vector 实现；IntSet 不是 vector。
};
int main() {
    IntSet values;
    const bool first = values.insert(7);
    const bool duplicate = values.insert(7);
    const bool second = values.insert(9);
    std::cout << std::boolalpha << first << ", " << duplicate << ", " << second << '\n';
    std::cout << "size=" << values.size() << '\n';
    assert(first && !duplicate && second && values.size() == 2);
    // values.push_back(7); // 不存在这个接口，外部无法绕过唯一性检查。
}

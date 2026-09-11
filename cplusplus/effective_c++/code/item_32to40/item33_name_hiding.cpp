#include <iostream>

struct Base {
    void show(int) const { std::cout << "Base::show(int)\n"; }
};
struct Hidden : Base {
    void show(double) const { std::cout << "Hidden::show(double)\n"; }
};
struct Visible : Base {
    using Base::show; // 将基类重载引入当前作用域。
    void show(double) const { std::cout << "Visible::show(double)\n"; }
};
int main() {
    Hidden hidden;
    hidden.show(3); // 基类 int 重载被隐藏，3 转为 double。
    Visible visible;
    visible.show(3); // 精确匹配 Base::show(int)。
    visible.show(3.5);
    hidden.Base::show(3); // 也可显式指定基类成员。
}

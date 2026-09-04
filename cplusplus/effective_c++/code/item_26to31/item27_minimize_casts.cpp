#include <iostream>

class Animal {
public:
    virtual ~Animal() = default; // 让 Animal 成为多态基类
};

class Dog final : public Animal {
public:
    void fetch() const { std::cout << "Dog fetches\n"; }
};

class Cat final : public Animal {};

void tryFetch(Animal& animal) {
    if (auto* dog = dynamic_cast<Dog*>(&animal)) {
        dog->fetch();
    } else {
        std::cout << "not a Dog\n";
    }
}

int main() {
    Dog dog;
    Cat cat;
    tryFetch(dog);
    tryFetch(cat);

    int count = 7;
    const double average = static_cast<double>(count) / 2; // 明确数值转换
    std::cout << "average=" << average << '\n';

    // static_cast<Dog&>(cat).fetch(); // 错误思路：向下 static_cast 不检查真实类型
}

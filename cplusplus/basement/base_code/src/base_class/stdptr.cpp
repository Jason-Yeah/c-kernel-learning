#include <iostream>
#include <memory>
#include <string>

class Animal {
public:
    std::string name;
    Animal(std::string n) : name(n) {}
    virtual void speak() { std::cout << name << " 发出声音" << std::endl; }
    virtual ~Animal() = default; // 好习惯：为多态基类定义虚析构函数
};

class Dog : public Animal {
public:
    Dog(std::string n) : Animal(n) {}
    void speak() override { std::cout << name << " 汪汪汪！" << std::endl; }
};

// 一个工厂函数，返回一个 unique_ptr
std::unique_ptr<Animal> createAnimal(const std::string& type, const std::string& name) {
    if (type == "dog") {
        return std::make_unique<Dog>(name); // make_unique 是创建 unique_ptr 的安全方式
    }
    // ...可以处理其他动物类型
    return nullptr;
}

int main() {
    // 1. 从工厂获取一个独一无二的动物对象
    std::unique_ptr<Animal> myPet = createAnimal("dog", "旺财");
    
    // 2. 像普通指针一样使用它
    myPet->speak(); // 输出: 旺财 汪汪汪！

    // 3. 所有权转移 (move)
    std::unique_ptr<Animal> anotherPet = std::move(myPet);
    
    // myPet 现在为空了，不能再使用
    // anotherPet 现在是旺财的唯一所有者
    
    // 4. 当 anotherPet 离开作用域时，它会自动 delete 掉指向的 Dog 对象
    // 无需手动调用 delete!
    return 0;
}
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Child; // 前向声明

class Parent {
public:
    std::string name;
    std::vector<std::shared_ptr<Child>> children; // 父节点拥有子节点

    Parent(std::string n) : name(n) { std::cout << "Parent " << name << " 创建" << std::endl; }
    ~Parent() { std::cout << "Parent " << name << " 销毁" << std::endl; }
};

class Child {
public:
    std::string name;
    // ❌ 错误示范：如果用 shared_ptr，会形成循环引用
    // std::shared_ptr<Parent> parent; 
    
    // ✅ 正确做法：使用 weak_ptr 指向父节点
    std::weak_ptr<Parent> parent; // 子节点只“观察”父节点，不拥有它

    Child(std::string n) : name(n) { std::cout << "Child " << name << " 创建" << std::endl; }
    ~Child() { std::cout << "Child " << name << " 销毁" << std::endl; }
    
    void showParentName() {
        // 使用 weak_ptr 时，需要先 lock() 来获取一个临时的 shared_ptr
        if (auto p = parent.lock()) { 
            std::cout << "我的父亲是: " << p->name << std::endl;
        } else {
            std::cout << "我的父亲已经不存在了" << std::endl;
        }
    }
};

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

class DataCache {
public:
    std::string data;
    DataCache(std::string d) : data(d) { std::cout << "DataCache 创建: " << data << std::endl; }
    ~DataCache() { std::cout << "DataCache 销毁: " << data << std::endl; }
};

class User {
private:
    std::string name;
    std::shared_ptr<DataCache> cache; // 用户共享同一个缓存
public:
    User(std::string n, std::shared_ptr<DataCache> c) : name(n), cache(c) {}
    
    void showData() {
        std::cout << "用户 " << name << " 读取数据: " << cache->data << std::endl;
    }
};

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

    anotherPet->speak();

    // =========

     // 1. 创建一个共享的缓存对象
    auto sharedCache = std::make_shared<DataCache>("热点数据");
    std::cout << "引用计数: " << sharedCache.use_count() << std::endl; // 计数为 1

    {
        // 2. 创建两个用户，他们都共享这份缓存
        User user1("Alice", sharedCache);
        User user2("Bob", sharedCache);
        
        std::cout << "引用计数: " << sharedCache.use_count() << std::endl; // 计数为 3 (sharedCache, user1, user2)

        user1.showData();
        user2.showData();
        
    } // user1 和 user2 在这里离开作用域并被销毁
    
    std::cout << "引用计数: " << sharedCache.use_count() << std::endl; // 计数回到 1
    std::cout << "缓存依然存在，因为 sharedCache 还在持有它" << std::endl;

    //=================

    auto son = std::make_shared<Child>("小张");
    auto father = std::make_shared<Parent>("张三");
    

    // 建立关系
    father->children.push_back(son); // 父亲拥有儿子
    son->parent = father;            // 儿子观察父亲 (不会增加父亲的引用计数)

    son->showParentName(); // 输出: 我的父亲是: 张三

    // 当 main 函数结束时，father 和 son 的引用计数都变为0，都会被正确销毁

    return 0;
}
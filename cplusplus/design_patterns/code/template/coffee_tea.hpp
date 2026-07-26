#pragma once

#include <iostream>

class CaffeineBeverage // 咖啡因饮料
{
public:
    virtual ~CaffeineBeverage() = default;

    void prepare_recipe()
    {
        boil_water();
        brew();
        pour_in_cup();
        add_condiments();

        if (cutomer_wants_extra())
            extra_step();
    }

protected:
    void boil_water() { std::cout << "[PUBLIC] 100°C\n"; }

    void pour_in_cup() { std::cout << "[PUBLIC] Pour in cup.\n"; }

    virtual void brew() = 0;
    virtual void add_condiments() = 0;

    virtual bool cutomer_wants_extra() { return false; }
    virtual void extra_step() {}
};

class Coffee : public CaffeineBeverage
{
    // 声明为protected表示Client永远不能调用这些方法，只能通过基类的
    // `prepare_recipe()`方法调用抽象的过程，细节不公开。
protected:
    void brew() override { std::cout << "[COFFE] 冲咖啡粉\n"; }

    void add_condiments() override { std::cout << "[COFFEE] Add milk\n"; }

    bool cutomer_wants_extra() override { return true; }

    void extra_step() override { puts("[COFFEE] 拉花"); }
};

class Tea : public CaffeineBeverage
{
protected:
    void brew() override { std::cout << "[TEA] 冲茶叶\n"; }

    void add_condiments() override { std::cout << "[TEA] 爱加啥加啥\n"; }
};

#pragma once

#include <memory>
#include <string>

// ============ 抽象组件 (Component) ============
class Beverage {
public:
    virtual ~Beverage() = default;
    virtual std::string GetDescription() const = 0;
    virtual double Cost() const = 0;
};

// ============ 具体组件 (ConcreteComponent) ============
class Espresso : public Beverage {
public:
    std::string GetDescription() const override;
    double Cost() const override;

private:
    static constexpr double kPrice = 5.0;
};

class Americano : public Beverage {
public:
    std::string GetDescription() const override;
    double Cost() const override;

private:
    static constexpr double kPrice = 4.0;
};

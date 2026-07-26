#pragma once

#include "beverage.hpp"

// ============ 抽象装饰 (Decorator) ============
class CondimentDecorator : public Beverage {
protected:
    std::unique_ptr<Beverage> beverage_;

public:
    explicit CondimentDecorator(std::unique_ptr<Beverage> beverage);
};

// ============ 具体装饰 (ConcreteDecorator) ============
class Sugar : public CondimentDecorator {
public:
    using CondimentDecorator::CondimentDecorator;
    std::string GetDescription() const override;
    double Cost() const override;

private:
    static constexpr double kPrice = 0.5;
};

class Milk : public CondimentDecorator {
public:
    using CondimentDecorator::CondimentDecorator;
    std::string GetDescription() const override;
    double Cost() const override;

private:
    static constexpr double kPrice = 1.5;
};

class Whip : public CondimentDecorator {
public:
    using CondimentDecorator::CondimentDecorator;
    std::string GetDescription() const override;
    double Cost() const override;

private:
    static constexpr double kPrice = 1.0;
};

class DoubleSugar : public CondimentDecorator {
public:
    using CondimentDecorator::CondimentDecorator;
    std::string GetDescription() const override;
    double Cost() const override;

private:
    static constexpr double kPrice = 1.0;
};

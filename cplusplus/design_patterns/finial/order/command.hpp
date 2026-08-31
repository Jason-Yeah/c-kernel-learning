#pragma once

#include "state.hpp"
#include "facade.hpp"


class Command
{
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

class CheckoutCommand final : public Command
{
public:
    CheckoutCommand(ShopService &shop, Order &order,
                    std::unique_ptr<PricingStrategy> pricing)
        : shop_(shop), order_(order), pricing_(std::move(pricing))
    {
    }

    void execute() override
    {
        if (executed_)
            throw std::logic_error("命令不能重复执行");
        shop_.checkout(order_, std::move(pricing_));
        executed_ = true;
    }

    void undo() override
    {
        if (!executed_)
            throw std::logic_error("尚未执行，不能撤销");
        order_.cancel();
        executed_ = false;
    }

private:
    ShopService &shop_;
    Order &order_;
    std::unique_ptr<PricingStrategy> pricing_;
    bool executed_ = false;
};

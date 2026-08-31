#pragma once

#include "state.hpp"
#include <memory>

class OrderValidator
{
public:
    virtual ~OrderValidator() = default;

    OrderValidator &setNext(std::unique_ptr<OrderValidator> next)
    {
        next_ = std::move(next);
        return *next_;
    }

    void validate(const Order &order) const
    {
        check(order);
        if (next_)
            next_->validate(order);
    }

private:
    virtual void check(const Order &) const = 0;
    std::unique_ptr<OrderValidator> next_;
};

class NonEmptyValidator final : public OrderValidator
{
    void check(const Order &order) const override
    {
        if (order.items().empty())
            throw std::runtime_error("购物车不能为空");
    }
};

class StockValidator final : public OrderValidator
{
    void check(const Order &order) const override
    {
        for (const auto &item : order.items())
        {
            if (item.quantity <= 0 || item.quantity > item.stock)
            {
                throw std::runtime_error(item.name + " 库存不足或数量非法");
            }
        }
    }
};
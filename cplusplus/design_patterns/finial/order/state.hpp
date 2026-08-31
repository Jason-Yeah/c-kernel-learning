#pragma once

#include "observer.hpp"
#include "strategy.hpp"
#include <memory>
#include <string>
#include <vector>

class Order;

class OrderState
{
public:
    virtual ~OrderState() = default;
    virtual const char *name() const = 0;
    virtual void submit(Order &) const;
    virtual void pay(Order &) const;
    virtual void ship(Order &) const;
    virtual void cancel(Order &) const;
};

class DraftState final : public OrderState
{
public:
    const char *name() const override { return "草稿"; }
    void submit(Order &) const override;
    void cancel(Order &) const override;
};

class AwaitingPaymentState final : public OrderState
{
public:
    const char *name() const override { return "待支付"; }
    void pay(Order &) const override;
    void cancel(Order &) const override;
};

class PaidState final : public OrderState
{
public:
    const char *name() const override { return "已支付"; }
    void ship(Order &) const override;
    void cancel(Order &) const override;
};

class ShippedState final : public OrderState
{
public:
    const char *name() const override { return "已发货"; }
};

class CancelledState final : public OrderState
{
public:
    const char *name() const override { return "已取消"; }
};

struct OrderItem
{
    std::string name;
    int quantity;
    double unitPrice;
    int stock;
};

class Order
{
private:
    std::string id_;
    std::vector<OrderItem> items_;
    std::unique_ptr<OrderState> state_; // Only one
    std::unique_ptr<PricingStrategy> pricing_;
    std::vector<OrderObserver *> observers_; // 观察者生命周期由客户端管理

public:
    Order(std::string id, std::vector<OrderItem> items)
        : id_(std::move(id)), items_(std::move(items)), state_(new DraftState),
          pricing_(new RegularPricing)
    {
    }

    const std::string &id() const { return id_; }
    const std::vector<OrderItem> &items() const { return items_; }
    const std::string stateName() const { return state_->name(); }

    double originalAmount() const
    {
        double result = 0.0;
        for (const auto &item : items_)
        {
            result += item.quantity * item.unitPrice;
        }
        return result;
    }

    double payableAmount() const
    {
        return pricing_->calculate(originalAmount());
    }

    void setPricing(std::unique_ptr<PricingStrategy> strategy)
    {
        pricing_ = std::move(strategy);
    }

    void addObserver(OrderObserver &observer)
    {
        observers_.push_back(&observer);
    }

    void transitionTo(std::unique_ptr<OrderState> next)
    {
        state_ = std::move(next);
        for (auto *observer : observers_)
        {
            observer->onStateChanged(id_, state_->name());
        }
    }

    void submit() { state_->submit(*this); }
    void pay() { state_->pay(*this); }
    void ship() { state_->ship(*this); }
    void cancel() { state_->cancel(*this); }
};

//

void OrderState::submit(Order &) const
{
    throw std::logic_error("当前状态不能提交");
}
void OrderState::pay(Order &) const
{
    throw std::logic_error("当前状态不能支付");
}
void OrderState::ship(Order &) const
{
    throw std::logic_error("当前状态不能发货");
}
void OrderState::cancel(Order &) const
{
    throw std::logic_error("当前状态不能取消");
}

//

void DraftState::submit(Order &order) const
{
    order.transitionTo(std::unique_ptr<OrderState>(new AwaitingPaymentState));
}
void DraftState::cancel(Order &order) const
{
    order.transitionTo(std::unique_ptr<OrderState>(new CancelledState));
}
void AwaitingPaymentState::pay(Order &order) const
{
    order.transitionTo(std::unique_ptr<OrderState>(new PaidState));
}
void AwaitingPaymentState::cancel(Order &order) const
{
    order.transitionTo(std::unique_ptr<OrderState>(new CancelledState));
}
void PaidState::ship(Order &order) const
{
    order.transitionTo(std::unique_ptr<OrderState>(new ShippedState));
}
void PaidState::cancel(Order &order) const
{
    std::cout << "[退款] 原支付将在原渠道退回\n";
    order.transitionTo(std::unique_ptr<OrderState>(new CancelledState));
}



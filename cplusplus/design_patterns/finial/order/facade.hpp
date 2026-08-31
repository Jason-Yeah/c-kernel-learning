#pragma once

#include "adapter.hpp"
#include "cor.hpp"
#include <iomanip>
#include <iostream>

class ShopService
{
public:
    ShopService(const OrderValidator &validator, PaymentGateway &gateway)
        : validator_(validator), gateway_(gateway)
    {
    }

    // 包装成一个简单入口
    void checkout(Order &order, std::unique_ptr<PricingStrategy> pricing)
    {
        validator_.validate(order);
        order.setPricing(std::move(pricing));
        std::cout << std::fixed << std::setprecision(2) << "原价："
                  << order.originalAmount() << "，应付："
                  << order.payableAmount() << '\n';
        order.submit();
        if (!gateway_.pay(order.id(), order.payableAmount()))
        {
            order.cancel();
            throw std::runtime_error("支付失败");
        }
        order.pay();
    }

private:
    const OrderValidator &validator_;
    PaymentGateway &gateway_;
};
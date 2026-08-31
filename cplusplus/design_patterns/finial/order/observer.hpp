#pragma once

#include <iostream>

class Order;

class OrderObserver
{
public:
    virtual ~OrderObserver() = default;
    virtual void onStateChanged(const std::string &orderId,
                                const std::string &state) = 0;
};

class SmsObserver final : public OrderObserver
{
public:
    void onStateChanged(const std::string &id,
                        const std::string &state) override
    {
        std::cout << "[短信] 订单 " << id << " 状态变为：" << state << '\n';
    }
};

class LogObserver final : public OrderObserver
{
public:
    void onStateChanged(const std::string &id,
                        const std::string &state) override
    {
        std::cout << "[日志] order=" << id << ", state=" << state << '\n';
    }
};
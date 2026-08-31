#pragma once

#include <iostream>

class PaymentGateway
{
public:
    virtual ~PaymentGateway() = default;
    virtual bool pay(const std::string &orderId, double amount) = 0;
};

class LegacyPaySdk
{
public:
    int makePayment(const std::string &tradeNo, long cents)
    {
        std::cout << "[第三方支付] trade=" << tradeNo << ", cents=" << cents
                  << '\n';
        return 0; // 第三方约定：0 表示成功
    }
};

class LegacyPayAdapter final : public PaymentGateway
{
public:
    explicit LegacyPayAdapter(LegacyPaySdk &sdk) : sdk_(sdk) {}

    bool pay(const std::string &orderId, double amount) override
    {
        return sdk_.makePayment(orderId,
                                static_cast<long>(amount * 100 + 0.5)) == 0;
    }

private:
    LegacyPaySdk &sdk_;
};

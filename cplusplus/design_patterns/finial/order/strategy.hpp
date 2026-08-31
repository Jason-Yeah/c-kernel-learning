#pragma once

class PricingStrategy
{
public:
    virtual ~PricingStrategy() = default;
    virtual double calculate(double original) const = 0;
};

class RegularPricing final : public PricingStrategy
{
public:
    double calculate(double original) const override { return original; }
};

// VIP 默认9折
class VipPricing final : public PricingStrategy
{
public:
    double calculate(double original) const override { return original * 0.9; }
};
#pragma once

class CashSuper
{
public:
    virtual ~CashSuper() = default;
    virtual double acceptCash(double money) = 0;
};

#pragma once

#include "cash_super.hpp"

class CashReturn : public CashSuper
{
private:
    double _money_cond = 0;
    double _money_ret = 0;

public:
    CashReturn(const double money_cond, const double money_ret)
        : _money_cond(money_cond), _money_ret(money_ret)
    {
    }

    double acceptCash(double money) override
    {
        double res = money;
        if (money >= _money_cond)
            res -= (money / _money_cond) * _money_ret;
        return res;
    }
};

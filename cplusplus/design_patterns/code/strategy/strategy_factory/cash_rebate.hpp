#pragma once

#include "cash_super.hpp"

class CashRebate: public CashSuper
{
private:
    double _money_rebate = 1;

public:
    CashRebate(const double money_rebate) : _money_rebate(money_rebate) {}

    double acceptCash(double money) override { return money * _money_rebate; }
};

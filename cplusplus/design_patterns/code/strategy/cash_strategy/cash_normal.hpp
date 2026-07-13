#pragma once

#include "cash_super.hpp"

class CashNormal: public CashSuper
{
public:
    double acceptCash(double money) override { return money; }
};

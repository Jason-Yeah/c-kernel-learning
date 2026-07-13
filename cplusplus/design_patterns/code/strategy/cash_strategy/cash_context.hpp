#pragma once

#include "cash_super.hpp"
#include <memory>

class CashContext
{
private:
    std::unique_ptr<CashSuper> _cs;

public:
    explicit CashContext(std::unique_ptr<CashSuper> cs)
        : _cs(std::move(cs)) {}

    double getRes(double money) { return _cs->acceptCash(money); }
};

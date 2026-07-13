#pragma once

#include "cash_normal.hpp"
#include "cash_rebate.hpp"
#include "cash_return.hpp"
#include "cash_super.hpp"
#include <memory>
#include <stdexcept>
#include <string>

class CashContext
{
private:
    std::unique_ptr<CashSuper> _cs;

public:
    explicit CashContext(std::string type)
    {
        if (type == "正常收费")
            _cs = std::make_unique<CashNormal>();
        else if (type == "打八折")
            _cs = std::make_unique<CashRebate>(0.8);
        else if (type == "满300返100")
            _cs = std::make_unique<CashReturn>(300, 100);
        else
            throw std::invalid_argument("Unknown cash type: " + type);
    }

    double getRes(double money) { return _cs->acceptCash(money); }
};

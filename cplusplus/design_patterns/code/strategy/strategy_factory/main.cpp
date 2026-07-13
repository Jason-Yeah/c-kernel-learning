#include "cash_context.hpp"
#include <iostream>
#include <memory>

int main()
{
    // ========== 策略 1：正常收费 ==========
    CashContext cc("打八折");

    double total = cc.getRes(100);

    std::cout << total << std::endl;

    return 0;
}
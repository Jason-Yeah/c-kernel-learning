#include "cash_context.hpp"
#include "cash_normal.hpp"
#include "cash_rebate.hpp"
#include "cash_return.hpp"
#include <iostream>
#include <memory>

int main()
{
    // ========== 策略 1：正常收费 ==========
    auto ctx1 = CashContext(std::make_unique<CashNormal>());
    std::cout << "正常收费 100  → " << ctx1.getRes(100) << std::endl;

    // ========== 策略 2：打 8 折 ==========
    auto ctx2 = CashContext(std::make_unique<CashRebate>(0.8));
    std::cout << "打 8 折 100   → " << ctx2.getRes(100) << std::endl;

    // ========== 策略 3：满 300 减 100 ==========
    auto ctx3 = CashContext(std::make_unique<CashReturn>(300, 100));
    std::cout << "满 300 减 100 (200) → " << ctx3.getRes(200) << std::endl;
    std::cout << "满 300 减 100 (600) → " << ctx3.getRes(600) << std::endl;

    return 0;
}
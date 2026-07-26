#include "condiment.hpp"
#include <iostream>

int main() {
    // 浓缩咖啡
    auto b1 = std::make_unique<Espresso>();
    std::cout << b1->GetDescription() << "  ￥" << b1->Cost() << std::endl;

    // 浓缩 + 奶
    auto b2 = std::make_unique<Milk>(std::make_unique<Espresso>());
    std::cout << b2->GetDescription() << "  ￥" << b2->Cost() << std::endl;

    // 浓缩 + 双倍糖 + 奶 + 奶油
    auto b3 = std::make_unique<Whip>(
        std::make_unique<Milk>(
            std::make_unique<DoubleSugar>(
                std::make_unique<Espresso>())));
    std::cout << b3->GetDescription() << "  ￥" << b3->Cost() << std::endl;

    // 美式 + 糖 + 奶
    auto b4 = std::make_unique<Milk>(
        std::make_unique<Sugar>(std::make_unique<Americano>()));
    std::cout << b4->GetDescription() << "  ￥" << b4->Cost() << std::endl;

    return 0;
}

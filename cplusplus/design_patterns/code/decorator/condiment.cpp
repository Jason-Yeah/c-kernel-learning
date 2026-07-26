#include "condiment.hpp"

// ============ CondimentDecorator ============
CondimentDecorator::CondimentDecorator(std::unique_ptr<Beverage> beverage)
    : beverage_(std::move(beverage)) {}

// ============ Sugar ============
std::string Sugar::GetDescription() const {
    return beverage_->GetDescription() + " + 糖";
}

double Sugar::Cost() const {
    return beverage_->Cost() + kPrice;
}

// ============ Milk ============
std::string Milk::GetDescription() const {
    return beverage_->GetDescription() + " + 奶";
}

double Milk::Cost() const {
    return beverage_->Cost() + kPrice;
}

// ============ Whip ============
std::string Whip::GetDescription() const {
    return beverage_->GetDescription() + " + 奶油";
}

double Whip::Cost() const {
    return beverage_->Cost() + kPrice;
}

// ============ DoubleSugar ============
std::string DoubleSugar::GetDescription() const {
    return beverage_->GetDescription() + " + 双倍糖";
}

double DoubleSugar::Cost() const {
    return beverage_->Cost() + kPrice;
}

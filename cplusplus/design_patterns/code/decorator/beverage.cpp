#include "beverage.hpp"

// ============ Espresso ============
std::string Espresso::GetDescription() const {
    return "浓缩咖啡";
}

double Espresso::Cost() const {
    return kPrice;
}

// ============ Americano ============
std::string Americano::GetDescription() const {
    return "美式咖啡";
}

double Americano::Cost() const {
    return kPrice;
}

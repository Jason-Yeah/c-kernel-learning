#pragma once

#include "order.hpp"
#include <vector>

class Cart
{
public:
    void addOrder(Order order)
    {
        orders.push_back(order);
    }
    double sum()
    {
        double res = 0;
        for (auto& order: orders)
            res += order.sum();
        return res;
    }
    
private:
    std::vector<Order> orders;
};
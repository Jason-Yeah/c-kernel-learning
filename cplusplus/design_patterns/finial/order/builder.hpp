#pragma once

#include "state.hpp"
#include <memory>
#include <string>

class OrderBuilder
{
public:
    explicit OrderBuilder(std::string id) : id_(std::move(id)) {}

    OrderBuilder &addItem(std::string name, int quantity, double unitPrice,
                          int stock)
    {
        items_.push_back({std::move(name), quantity, unitPrice, stock});
        return *this;
    }

    std::unique_ptr<Order> build()
    {
        return std::unique_ptr<Order>(
            new Order(std::move(id_), std::move(items_)));
    }

private:
    std::string id_;
    std::vector<OrderItem> items_;
};
#pragma once

#include "operation.hpp"
#include <memory>
#include <stdexcept>
#include <string>

class OperatorFactory
{
public:
    static std::unique_ptr<Operator> createOperate(const std::string &op)
    {
        if (op == "+")
            return std::make_unique<OperatorAdd>();
        else if (op == "-")
            return std::make_unique<OperatorSub>();

        throw std::invalid_argument("Unknown operator: " + op);
    }
};

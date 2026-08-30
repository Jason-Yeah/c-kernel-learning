#pragma once

#include <iostream>
#include <map>
#include <memory>

class Context
{
    std::map<std::string, int> variables_;

public:
    void SetVariable(const std::string &name, int value)
    {
        variables_[name] = value;
    }

    int GetVariable(const std::string &name) const
    {
        auto it = variables_.find(name);
        return it != variables_.end() ? it->second : 0;
    }
};

class Expression
{
public:
    virtual ~Expression() = default;

    virtual int Interpret(const Context &context) const = 0;
};

class NumberExpression : public Expression
{
    int value_;

public:
    explicit NumberExpression(const int value) : value_(value) {}

    int Interpret(const Context &) const override { return value_; }
};

class VariableExpression : public Expression
{
    std::string name_;

public:
    explicit VariableExpression(std::string name) : name_(std::move(name)) {}

    int Interpret(const Context &context) const override
    {
        return context.GetVariable(name_);
    }
};

class AddExpression : public Expression
{
    using exp = std::unique_ptr<Expression>;
    exp left_;
    exp right_;

public:
    AddExpression(exp l, exp r) : left_(std::move(l)), right_(std::move(r)) {}

    int Interpret(const Context &context) const override
    {
        return left_->Interpret(context) + right_->Interpret(context);
    }
};

class MultiplyExpression : public Expression
{
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;

public:
    MultiplyExpression(std::unique_ptr<Expression> left,
                       std::unique_ptr<Expression> right)
        : left_(std::move(left)), right_(std::move(right))
    {
    }

    int Interpret(const Context &context) const override
    {
        return left_->Interpret(context) * right_->Interpret(context);
    }
};

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

class Context;

class Expression
{
public:
    virtual ~Expression() = default;

    virtual int Interpret(const Context &context) const = 0;
};

class Context
{
public:
    void SetVariable(const std::string &name, int value)
    {
        variables_[name] = value;
    }

    int GetVariable(const std::string &name) const
    {
        auto it = variables_.find(name);

        if (it == variables_.end())
        {
            throw std::runtime_error("Variable not found: " + name);
        }

        return it->second;
    }

private:
    std::unordered_map<std::string, int> variables_;
};

class NumberExpression : public Expression
{
public:
    explicit NumberExpression(int value) : value_(value) {}

    int Interpret(const Context &) const override { return value_; }

private:
    int value_;
};

class VariableExpression : public Expression
{
public:
    explicit VariableExpression(std::string name) : name_(std::move(name)) {}

    int Interpret(const Context &context) const override
    {
        return context.GetVariable(name_);
    }

private:
    std::string name_;
};

class AddExpression : public Expression
{
public:
    AddExpression(std::unique_ptr<Expression> left,
                  std::unique_ptr<Expression> right)
        : left_(std::move(left)), right_(std::move(right))
    {
    }

    int Interpret(const Context &context) const override
    {
        return left_->Interpret(context) + right_->Interpret(context);
    }

private:
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class MultiplyExpression : public Expression
{
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

private:
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

int main()
{
    Context context;

    context.SetVariable("x", 3);

    auto tree = std::make_unique<MultiplyExpression>(
        std::make_unique<VariableExpression>("x"),
        std::make_unique<AddExpression>(std::make_unique<NumberExpression>(4),
                                        std::make_unique<NumberExpression>(5)));

    std::cout << tree->Interpret(context) << '\n';
}

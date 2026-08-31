#pragma once

#include "composite.hpp"
#include "visitor.hpp"
#include <sstream>
#include <string>

class RuleExpression
{
public:
    virtual ~RuleExpression() = default;
    virtual bool interpret(const File &) const = 0;
    virtual std::string describe() const = 0;
};

class ExtensionIs final : public RuleExpression
{
public:
    explicit ExtensionIs(std::string extension)
        : extension_(std::move(extension))
    {
    }

    bool interpret(const File &file) const override
    {
        return file.type().extension() == extension_;
    }
    std::string describe() const override { return "扩展名为 " + extension_; }

private:
    std::string extension_;
};



class SizeGreaterThan final : public RuleExpression
{
public:
    explicit SizeGreaterThan(std::size_t kb) : kb_(kb) {}

    bool interpret(const File &file) const override
    {
        return file.sizeKb() > kb_;
    }
    std::string describe() const override
    {
        return "大小超过 " + std::to_string(kb_) + "KB";
    }

private:
    std::size_t kb_;
};

class AndExpression final : public RuleExpression
{
public:
    AndExpression(std::unique_ptr<RuleExpression> left,
                  std::unique_ptr<RuleExpression> right)
        : left_(std::move(left)), right_(std::move(right))
    {
    }

    bool interpret(const File &file) const override
    {
        return left_->interpret(file) && right_->interpret(file);
    }
    std::string describe() const override
    {
        return "(" + left_->describe() + " AND " + right_->describe() + ")";
    }

private:
    std::unique_ptr<RuleExpression> left_;
    std::unique_ptr<RuleExpression> right_;
};

class StatisticsVisitor final : public NodeVisitor
{
public:
    void visit(const File &file) override
    {
        ++files_;
        totalKb_ += file.sizeKb();
    }
    void visit(const Directory &) override { ++directories_; }

    std::string result() const
    {
        std::ostringstream out;
        out << "目录数=" << directories_ << ", 文件数=" << files_
            << ", 总大小=" << totalKb_ << "KB";
        return out.str();
    }

private:
    std::size_t files_ = 0;
    std::size_t directories_ = 0;
    std::size_t totalKb_ = 0;
};

class RiskVisitor final : public NodeVisitor
{
public:
    explicit RiskVisitor(const RuleExpression &rule) : rule_(rule) {}

    void visit(const File &file) override
    {
        if (rule_.interpret(file))
            risks_.push_back(file.name());
    }

    void visit(const Directory &) override {}

    const std::vector<std::string> &risks() const { return risks_; }

private:
    const RuleExpression &rule_;
    std::vector<std::string> risks_;
};

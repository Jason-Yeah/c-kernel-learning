#pragma once

#include "flyweight.hpp"
#include "visitor.hpp"
#include <memory>
#include <string>
#include <vector>

class Node
{
public:
    explicit Node(std::string name) : name_(std::move(name)) {}
    virtual ~Node() = default;
    const std::string &name() const { return name_; }
    virtual void accept(NodeVisitor &) const = 0;

private:
    std::string name_; // 外部状态：每个节点不同
};

class File final : public Node
{
public:
    File(std::string name, std::size_t sizeKb,
         std::shared_ptr<const FileType> type)
        : Node(std::move(name)), sizeKb_(sizeKb), type_(std::move(type))
    {
    }

    std::size_t sizeKb() const { return sizeKb_; }
    const FileType &type() const { return *type_; }
    void accept(NodeVisitor &visitor) const override { visitor.visit(*this); }

private:
    std::size_t sizeKb_;
    std::shared_ptr<const FileType> type_;
};

class Directory final : public Node
{
public:
    Directory(std::string name, bool sensitive = false)
        : Node(std::move(name)), sensitive_(sensitive)
    {
    }

    void add(std::unique_ptr<Node> child)
    {
        children_.push_back(std::move(child));
    }

    bool sensitive() const { return sensitive_; }
    
    const std::vector<std::unique_ptr<Node>> &children() const
    {
        return children_;
    }

    void accept(NodeVisitor &visitor) const override
    {
        visitor.visit(*this);
        for (const auto &child : children_)
            child->accept(visitor);
    }

private:
    bool sensitive_;
    std::vector<std::unique_ptr<Node>> children_;
};
#pragma once

#include "component.hpp"
#include <vector>
#include <iostream>

class Directory : public FileSysNode
{
    std::string name_;
    std::vector<std::unique_ptr<FileSysNode>> children_;

public:
    explicit Directory(const std::string &name) : name_(name) {}

    size_t GetSize() const override 
    {
        size_t total = 0;
        std::cout << "  [目录] " << name_ << "/ {" << std::endl;
        for (const auto& child: children_)
            total += child->GetSize();

        std::cout << "  } = " << total << "B" << std::endl;
        return total;
    }

    void Add(std::unique_ptr<FileSysNode> child) override
    {
        children_.push_back(std::move(child));
    }

    std::string GetName() const override { return name_; }
};

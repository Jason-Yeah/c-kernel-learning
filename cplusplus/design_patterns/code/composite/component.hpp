#pragma once

#include <memory>
#include <stdexcept>
#include <string>

class FileSysNode
{
public:
    virtual ~FileSysNode() = default;

    virtual size_t GetSize() const = 0;

    virtual void Add(std::unique_ptr<FileSysNode> child)
    {
        throw std::runtime_error("叶子节点不支持 Add");
    }

    virtual std::string GetName() const = 0;
};

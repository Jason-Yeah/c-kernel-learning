#pragma once

#include "component.hpp"
#include <iostream>

class File : public FileSysNode
{
    std::string name_;
    size_t size_;

public:
    File(const std::string &name, size_t size) : name_(name), size_(size) {}

    size_t GetSize() const override
    {
        std::cout << "    [文件] " << name_ << " = " << size_ << "B"
                  << std::endl;
        return size_;
    }

    std::string GetName() const override { return name_; }

    //  File 没有重写 Add() → 继承基类的抛异常版本
};
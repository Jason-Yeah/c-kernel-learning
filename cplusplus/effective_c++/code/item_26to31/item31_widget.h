#pragma once

#include <memory>
#include <string>

class Widget {
public:
    explicit Widget(std::string name);
    ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;

    std::string summary() const;

private:
    struct Impl;                  // 不暴露 vector、缓存等具体实现
    std::unique_ptr<Impl> impl_;  // 指针大小固定，完整 Impl 留在 .cpp
};

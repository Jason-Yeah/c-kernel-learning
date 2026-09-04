#include "item31_widget.h"

#include <sstream>
#include <utility>
#include <vector>

struct Widget::Impl {
    explicit Impl(std::string name) : name(std::move(name)), cache{1, 2, 3} {}

    std::string name;
    std::vector<int> cache; // 未来修改这里，不必改公开头文件
};

Widget::Widget(std::string name) : impl_(std::make_unique<Impl>(std::move(name))) {}
Widget::~Widget() = default; // 此处 Impl 已完整定义，unique_ptr 可正确析构
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;

std::string Widget::summary() const {
    std::ostringstream output;
    output << impl_->name << ", cache size=" << impl_->cache.size();
    return output.str();
}

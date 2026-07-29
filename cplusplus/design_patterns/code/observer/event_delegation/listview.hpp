#pragma once

#include "ui_element.hpp"
#include <functional>
#include <iostream>
#include <string>
#include <vector>

class ListView
{
    std::vector<ListItem> items_;
    std::function<void(int, const ListItem &)> on_item_click_;

public:
    ListView &add_item(const std::string id, const std::string data)
    {
        items_.emplace_back(id, data);
        return *this;
    }

    void on_item_click(std::function<void(int, const ListItem &)> handler)
    {
        on_item_click_ = handler;
    }

    void SimulateClick(int index)
    {
        if (index < 0 || index >= (int)items_.size())
            return;

        const auto &item = items_[index];
        std::cout << "[ListView] 子元素 #" << index << " (" << item.get_id()
                  << ") 被点击, 冒泡到父容器" << std::endl;
        // 委托回调：传入 index + 被点击的 item
        if (on_item_click_)
            on_item_click_(index, item);
    }
};

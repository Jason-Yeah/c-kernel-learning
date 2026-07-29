#pragma once

#include <string>

class UIElement
{
public:
    UIElement(const std::string &id) : id_(id) {}

    const std::string &get_id() const { return id_; }

    std::string get_type() const { return type_; }

protected:
    std::string id_;
    std::string type_ = "element";
};

class ListItem : public UIElement
{
public:
    ListItem(const std::string &id, const std::string &data)
        : UIElement(id), data_(data)
    {
        type_ = "item";
    }

    const std::string &data() const { return data_; }

private:
    std::string data_;
};

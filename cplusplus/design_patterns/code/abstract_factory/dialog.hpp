#pragma once

#include <iostream>

class Dialog
{
public:
    virtual ~Dialog() = default;
    virtual void show() const = 0;
};

class WinDialog : public Dialog
{
public:
    void show() const override
    {
        std::cout << "  [Windows 对话框] 显示模态窗口，带最小/最大/关闭按钮"
                  << std::endl;
    }
};

class LinuxDialog : public Dialog
{
public:
    void show() const override
    {
        std::cout << "  [Linux 对话框] 显示 GTK 对话框" << std::endl;
    }
};
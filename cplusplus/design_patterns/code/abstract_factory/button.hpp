#pragma once

#include <iostream>

class Button
{
public:
    virtual ~Button() = default;
    virtual void render() const = 0; // 渲染
};

// Splitting to other files.

class WinButton : public Button
{
public:
    void render() const override
    {
        std::cout << "  [Windows 按钮] 绘制直角边框 + 灰色背景" << std::endl;
    }
};

class LinuxButton : public Button
{
public:
    void render() const override
    {
        std::cout << "  [Linux 按钮] 绘制 GTK 风格按钮" << std::endl;
    }
};

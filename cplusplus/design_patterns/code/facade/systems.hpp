#pragma once

#include <iostream>

class DVDPlayer
{
public:
    void TurnOn() { std::cout << "  [DVD] 启动" << std::endl; }
    void TurnOff() { std::cout << "  [DVD] 关闭" << std::endl; }
    void Play(const std::string &movie)
    {
        std::cout << "  [DVD] 播放 \"" << movie << "\"" << std::endl;
    }
};

class Projector
{
public:
    void TurnOn() { std::cout << "  [投影仪] 启动" << std::endl; }
    void TurnOff() { std::cout << "  [投影仪] 关闭" << std::endl; }
    void SetWideScreen()
    {
        std::cout << "  [投影仪] 切换宽屏模式" << std::endl;
    }
};

class Amplifier
{
public:
    void TurnOn() { std::cout << "  [功放] 启动" << std::endl; }
    void TurnOff() { std::cout << "  [功放] 关闭" << std::endl; }
    void SetVolume(int level)
    {
        std::cout << "  [功放] 音量设置为 " << level << std::endl;
    }
};

class Screen
{
public:
    void Down() { std::cout << "  [屏幕] 放下" << std::endl; }
    void Up() { std::cout << "  [屏幕] 收起" << std::endl; }
};

class Lights
{
public:
    void Dim(int percent)
    {
        std::cout << "  [灯光] 调暗至 " << percent << "%" << std::endl;
    }
    void Restore() { std::cout << "  [灯光] 恢复亮度" << std::endl; }
};

class PopcornPopper
{
public:
    void TurnOn() { std::cout << "  [爆米花机] 开始爆米花" << std::endl; }
    void TurnOff() { std::cout << "  [爆米花机] 关闭" << std::endl; }
};

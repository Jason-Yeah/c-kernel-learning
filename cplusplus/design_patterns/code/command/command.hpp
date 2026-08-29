#pragma once

#include <iostream>

class Command
{
public:
    virtual ~Command() = default;
    virtual void exe() = 0;
    virtual void undo() = 0;
};

// Receiver
class TV
{
    int volume_ = 10;

public:
    void TurnOn() { std::cout << "  [TV] 开机" << std::endl; }
    void TurnOff() { std::cout << "  [TV] 关机" << std::endl; }
    void VolumeUp()
    {
        std::cout << "  [TV] 音量 +1 → " << ++volume_ << std::endl;
    }
    void VolumeDown()
    {
        std::cout << "  [TV] 音量 -1 → " << --volume_ << std::endl;
    }
};

class TurnOnCommand : public Command
{
    TV &tv_;

public:
    explicit TurnOnCommand(TV &tv) : tv_(tv) {}

    void exe() override { tv_.TurnOn(); }

    void undo() override { tv_.TurnOff(); }
};

class TurnOffCommand : public Command
{
    TV &tv_;

public:
    explicit TurnOffCommand(TV &tv) : tv_(tv) {}

    void exe() override { tv_.TurnOff(); }

    void undo() override { tv_.TurnOn(); }
};

// 模式角色：ConcreteCommand —— 音量+命令（带撤销）
class VolumeUpCommand : public Command
{
    TV &tv_;

public:
    explicit VolumeUpCommand(TV &tv) : tv_(tv) {}

    void exe() override { tv_.VolumeUp(); }

    void undo() override { tv_.VolumeDown(); } // 音量+的反操作是音量-
};

#include <memory>
#include <vector>

// Invoker
class RemoteControl
{
    std::vector<std::unique_ptr<Command>> history_; // 命令历史（用于撤销）

public:
    // 按下按钮 → 执行命令 + 记录历史
    void PressButton(std::unique_ptr<Command> cmd)
    {
        cmd->exe();
        history_.push_back(std::move(cmd)); // ★ 命令是对象，可以存起来！
    }

    // 撤销：从历史里取出最后一个命令，调它的 Undo
    void PressUndo()
    {
        if (history_.empty())
        {
            std::cout << "  [遥控器] 没有可撤销的操作" << std::endl;
            return;
        }
        auto &last = history_.back();
        last->undo();
        history_.pop_back(); // 撤销后从历史移除
    }
};
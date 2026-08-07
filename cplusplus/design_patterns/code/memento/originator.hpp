#pragma once

#include "memento.hpp"
#include <memory>
#include <string>

class Editor
{
    std::string txt_;
    int cursorPos_ = 0;

public:
    void Type(const std::string &input)
    {
        txt_.insert(cursorPos_, input);
        cursorPos_ += input.size();
    }

    void MoveCursor(int pos)
    {
        if (pos >= 0 && pos <= (int)txt_.size())
            cursorPos_ = pos;
    }

    std::unique_ptr<EditorMemento> CreatrMemento() const
    {
        return std::unique_ptr<EditorMemento>(
            new EditorMemento(txt_, cursorPos_));
    }

    void Restore(const EditorMemento &memento)
    {
        txt_ = memento.GetText();
        cursorPos_ = memento.GetCursorPos();
    }

    void Show() const
    {
        std::cout << "  文本: \"" << txt_ << "\"" << "  光标: " << cursorPos_
                  << std::endl;
    }
};
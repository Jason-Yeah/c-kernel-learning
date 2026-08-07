#pragma once

#include <iostream>
#include <string>

class EditorMemento
{
    friend class Editor;

    std::string txt_;
    int cursorPos_;

    explicit EditorMemento(const std::string &text, int pos)
        : txt_(text), cursorPos_(pos)
    {
    }

    std::string GetText() const { return txt_; }
    int GetCursorPos() const { return cursorPos_; }
};
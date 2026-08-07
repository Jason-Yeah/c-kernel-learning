#pragma once

#include "memento.hpp"
#include <memory>
#include <vector>

class History
{
    std::vector<std::unique_ptr<EditorMemento>> snapshots_;

public:
    void Push(std::unique_ptr<EditorMemento> snap)
    {
        snapshots_.push_back(std::move(snap));
    }

    std::unique_ptr<EditorMemento> Pop()
    {
        if (snapshots_.empty())
            return nullptr;
        auto snap = std::move(snapshots_.back());
        snapshots_.pop_back();
        return snap;
    }

    bool Empty() const { return snapshots_.empty(); }
};

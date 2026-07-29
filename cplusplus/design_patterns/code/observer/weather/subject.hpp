#pragma once

#include <algorithm>
#include <vector>

class Observer; // 前向声明 — 够声明指针了

class Subject
{
private:
    std::vector<Observer *>
        observers_; // ← 裸指针，Subject 不负责 Observer 的生死

public:
    virtual ~Subject() = default; // ← 显式声明析构，编译器不会在 .hpp 里内联

    void Attach(Observer *obs) { observers_.push_back(obs); }

    void Detach(Observer *obs)
    {
        auto it = std::remove(observers_.begin(), observers_.end(), obs);
        //   ↑ remove 要求元素类型匹配 → Observer* 对 Observer*，没问题
        observers_.erase(it, observers_.end());
    }

protected:
    void Notify();
};

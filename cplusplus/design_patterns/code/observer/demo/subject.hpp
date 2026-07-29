#pragma once

#include <vector>
#include <algorithm>

// 前向声明 —— 只告诉编译器"有这么个类"，不暴露 Observer 的具体内容
class Observer;

// ============ 被观察者基类 ============
class Subject {
    std::vector<Observer*> observers_;

public:
    void Attach(Observer* obs);
    void Detach(Observer* obs);

protected:
    void Notify();  // ← 只声明，不定义！（函数体在 .cpp 里）
};

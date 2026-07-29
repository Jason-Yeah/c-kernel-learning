#include "subject.hpp"
#include "observer.hpp"   // ← 这里才 include！因为写函数体需要 Observer 的完整定义

void Subject::Attach(Observer* obs) {
    observers_.push_back(obs);
}

void Subject::Detach(Observer* obs) {
    auto it = std::remove(observers_.begin(), observers_.end(), obs);
    observers_.erase(it, observers_.end());
}

void Subject::Notify() {
    for (auto* obs : observers_)
        obs->Update();     // ← 此时 Observer 已经完整定义了，可以调 Update()
}

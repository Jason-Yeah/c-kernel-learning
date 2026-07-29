#pragma once

// ============ 观察者接口——这个文件独立，不依赖任何其他文件 ============
class Observer {
public:
    virtual ~Observer() = default;
    virtual void Update() = 0;
};

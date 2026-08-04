#pragma once

#include <string>

class Document;

// 抽象状态 (State)：定义所有具体状态的公共接口。
// 纯虚函数强制每个状态都实现该状态下的行为，杜绝 if-else 漏分支。
class DocumentState {
public:
    virtual ~DocumentState() = default;

    virtual std::string GetName() const = 0;

    // 尝试编辑正文。返回 true 表示当前状态允许编辑（并已写入正文），
    // false 表示拒绝。Context 把结果透传给调用方，由调用方决定后续逻辑。
    virtual bool Edit(Document& doc, const std::string& text) = 0;

    virtual void Publish(Document& doc) = 0;
};

#pragma once

#include <memory>
#include <string>

class DocumentState;

// 上下文 (Context)：持有当前状态，把 Edit/Publish 全部委托给状态对象。
// 状态切换由各状态类自己负责（状态模式方式二）。
class Document {
public:
    explicit Document(std::unique_ptr<DocumentState> initial);

    void SetState(std::unique_ptr<DocumentState> next);

    // 委托给当前状态；仅当状态允许时才写入正文（返回 true）。
    bool Edit(const std::string& text);

    void Publish();

    // 供状态类在批准编辑时写入正文（仅状态相关代码调用）。
    void SetContent(const std::string& text);

    const std::string& GetContent() const;

    std::string GetStateName() const;

private:
    std::unique_ptr<DocumentState> state_;
    std::string content_;
};

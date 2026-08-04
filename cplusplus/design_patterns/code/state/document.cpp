#include "document.hpp"

#include <iostream>
#include <utility>

#include "document_state.hpp"

Document::Document(std::unique_ptr<DocumentState> initial)
    : state_(std::move(initial)) {
    std::cout << "[文档] 初始状态: " << state_->GetName() << std::endl;
}

void Document::SetState(std::unique_ptr<DocumentState> next) {
    if (!next) return;  // 防御空指针
    std::cout << "  [转换] " << state_->GetName()
              << " → " << next->GetName() << std::endl;
    state_ = std::move(next);
}

bool Document::Edit(const std::string& text) {
    return state_->Edit(*this, text);
}

void Document::Publish() {
    state_->Publish(*this);
}

void Document::SetContent(const std::string& text) {
    content_ = text;
}

const std::string& Document::GetContent() const {
    return content_;
}

std::string Document::GetStateName() const {
    return state_->GetName();
}

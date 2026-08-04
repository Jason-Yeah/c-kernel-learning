#include "draft_state.hpp"

#include <iostream>
#include <memory>

#include "document.hpp"
#include "review_state.hpp"

std::string DraftState::GetName() const {
    return "草稿";
}

bool DraftState::Edit(Document& doc, const std::string& text) {
    std::cout << "  [草稿] 可以自由编辑" << std::endl;
    doc.SetContent(text);  // 草稿允许编辑，写入正文
    return true;
}

void DraftState::Publish(Document& doc) {
    std::cout << "  [草稿] 提交审核" << std::endl;
    doc.SetState(std::make_unique<ReviewState>());
}

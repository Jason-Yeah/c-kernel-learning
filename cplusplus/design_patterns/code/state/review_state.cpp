#include "review_state.hpp"

#include <iostream>
#include <memory>

#include "document.hpp"
#include "published_state.hpp"

std::string ReviewState::GetName() const {
    return "审核中";
}

bool ReviewState::Edit(Document& /*doc*/, const std::string& /*text*/) {
    // 审核中只允许批注，正文改动被拒绝（不调用 SetContent）
    std::cout << "  [审核] 只能添加批注，不能修改正文" << std::endl;
    return false;
}

void ReviewState::Publish(Document& doc) {
    std::cout << "  [审核] 审核通过，正式发布" << std::endl;
    doc.SetState(std::make_unique<PublishedState>());
}

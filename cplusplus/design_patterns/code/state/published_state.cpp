#include "published_state.hpp"

#include <iostream>

#include "document.hpp"

std::string PublishedState::GetName() const {
    return "已发布";
}

bool PublishedState::Edit(Document& /*doc*/, const std::string& /*text*/) {
    std::cout << "  [已发布] ❌ 文档已发布，不可编辑" << std::endl;
    return false;
}

void PublishedState::Publish(Document& /*doc*/) {
    std::cout << "  [已发布] ❌ 文档已发布，无需重复操作" << std::endl;
}

#include <iostream>
#include <memory>

#include "document.hpp"
#include "draft_state.hpp"

int main() {
    Document doc(std::make_unique<DraftState>());

    std::cout << "\n=== 步骤 1：编辑草稿 ===" << std::endl;
    doc.Edit("第一版内容");

    std::cout << "\n=== 步骤 2：提审 ===" << std::endl;
    doc.Publish();  // 草稿 → 审核中

    std::cout << "\n=== 步骤 3：审核中尝试编辑（应被拒绝） ===" << std::endl;
    if (!doc.Edit("审核期间偷偷改")) {
        std::cout << "  （正文未被改动，当前内容: " << doc.GetContent() << "）" << std::endl;
    }

    std::cout << "\n=== 步骤 4：审核通过发布 ===" << std::endl;
    doc.Publish();  // 审核中 → 已发布

    std::cout << "\n=== 步骤 5：发布后尝试编辑（应被拒绝） ===" << std::endl;
    doc.Edit("偷偷改一下");

    std::cout << "\n=== 步骤 6：发布后尝试重新发布 ===" << std::endl;
    doc.Publish();


    std::cout << "\n test:" << doc.GetContent() << std::endl;

    return 0;
}

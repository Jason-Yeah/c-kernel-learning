#pragma once

#include <string>

#include "document_state.hpp"

class Document;

// 具体状态：草稿。允许自由编辑，Publish 后进入审核。
class DraftState : public DocumentState {
public:
    std::string GetName() const override;
    bool Edit(Document& doc, const std::string& text) override;
    void Publish(Document& doc) override;
};

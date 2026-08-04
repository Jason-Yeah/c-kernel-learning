#pragma once

#include <string>

#include "document_state.hpp"

class Document;

// 具体状态：已发布。禁止编辑、禁止重复发布（终态）。
class PublishedState : public DocumentState {
public:
    std::string GetName() const override;
    bool Edit(Document& doc, const std::string& text) override;
    void Publish(Document& doc) override;
};

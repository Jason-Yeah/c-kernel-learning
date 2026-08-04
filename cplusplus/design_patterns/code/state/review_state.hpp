#pragma once

#include <string>

#include "document_state.hpp"

class Document;

// 具体状态：审核中。只允许批注，禁止修改正文；审核通过后发布。
class ReviewState : public DocumentState {
public:
    std::string GetName() const override;
    bool Edit(Document& doc, const std::string& text) override;
    void Publish(Document& doc) override;
};

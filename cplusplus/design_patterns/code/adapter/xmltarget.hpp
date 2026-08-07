#pragma once

#include <string>

class XmlParser
{
    std::string xmlData_;

public:
    explicit XmlParser(const std::string &xml) : xmlData_(xml) {}

    // 老接口：返回 XML 格式
    std::string GetXml() const { return xmlData_; }
};
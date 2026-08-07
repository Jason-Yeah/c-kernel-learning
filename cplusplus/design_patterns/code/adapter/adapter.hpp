#pragma once

#include "xmltarget.hpp"
#include <iostream>
#include <memory>
#include <string>

class JsonDataProvider
{
public:
    virtual ~JsonDataProvider() = default;
    virtual std::string GetJson() const = 0;
};

class Xml2JsonAdapter : public JsonDataProvider
{
    std::unique_ptr<XmlParser> xmlParser_;

public:
    explicit Xml2JsonAdapter(std::unique_ptr<XmlParser> parser)
        : xmlParser_(std::move(parser))
    {
    }

    std::string GetJson() const override
    {
        std::string xml = xmlParser_->GetXml(); // 拿到 XML

        // ===== 简单模拟 XML → JSON 转换 =====
        std::string json;
        if (xml.find("<name>") != std::string::npos)
        {
            auto start = xml.find("<name>") + 6;
            auto end = xml.find("</name>");
            std::string name = xml.substr(start, end - start);
            json = "{\"name\": \"" + name + "\"}";
        }
        return json;
    }
};

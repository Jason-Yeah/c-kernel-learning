#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

class ReportExporter
{
public:
    virtual ~ReportExporter() = default;

    std::string exportReport(const std::vector<std::string> &risks) const
    {
        return header() + body(risks) + footer(); // 固定算法骨架
    }

private:
    virtual std::string header() const = 0;
    virtual std::string body(const std::vector<std::string> &) const = 0;
    virtual std::string footer() const = 0;
};

class TextReportExporter final : public ReportExporter
{
    std::string header() const override { return "=== 安全审计报告 ===\n"; }
    std::string body(const std::vector<std::string> &risks) const override
    {
        std::ostringstream out;
        for (const auto &name : risks)
            out << "- 风险文件：" << name << '\n';
        if (risks.empty())
            out << "- 未发现风险\n";
        return out.str();
    }
    std::string footer() const override { return "=== 报告结束 ===\n"; }
};

class JsonReportExporter final : public ReportExporter
{
    std::string header() const override { return "{\"risks\":["; }
    std::string body(const std::vector<std::string> &risks) const override
    {
        std::ostringstream out;
        for (std::size_t i = 0; i < risks.size(); ++i)
        {
            if (i != 0)
                out << ',';
            out << '\"' << risks[i] << '\"'; // 示例文件名不包含需转义字符
        }
        return out.str();
    }
    std::string footer() const override { return "]}\n"; }
};

class ExporterCreator
{
public:
    virtual ~ExporterCreator() = default;
    virtual std::unique_ptr<ReportExporter> create() const = 0;
};

class TextExporterCreator final : public ExporterCreator
{
public:
    std::unique_ptr<ReportExporter> create() const override
    {
        return std::unique_ptr<ReportExporter>(new TextReportExporter);
    }
};

class JsonExporterCreator final : public ExporterCreator
{
public:
    std::unique_ptr<ReportExporter> create() const override
    {
        return std::unique_ptr<ReportExporter>(new JsonReportExporter);
    }
};
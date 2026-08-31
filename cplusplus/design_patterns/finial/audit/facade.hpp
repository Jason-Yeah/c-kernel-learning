#pragma once

#include "proxy.hpp"
#include "template_factory_method.hpp"

class SecurityCenter
{
public:
    explicit SecurityCenter(const ScanService &scanner) : scanner_(scanner) {}

    std::string audit(const Directory &root, Role role,
                      const RuleExpression &rule,
                      const ExporterCreator &creator) const
    {
        auto risks = scanner_.scan(root, role, rule);
        auto exporter = creator.create();
        return exporter->exportReport(risks);
    }

private:
    const ScanService &scanner_;
};

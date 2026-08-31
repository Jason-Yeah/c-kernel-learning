#pragma once

#include "composite.hpp"
#include "interpreter.hpp"
#include <iostream>
#include <string>
#include <vector>

enum class Role
{
    Employee,
    Admin
};

class ScanService
{
public:
    virtual ~ScanService() = default;
    virtual std::vector<std::string> scan(const Directory &, Role,
                                          const RuleExpression &) const = 0;
};

class RealScanService final : public ScanService
{
public:
    std::vector<std::string> scan(const Directory &root, Role,
                                  const RuleExpression &rule) const override
    {
        RiskVisitor visitor(rule);
        root.accept(visitor);
        return visitor.risks();
    }
};

class SecureScanProxy final : public ScanService
{
public:
    explicit SecureScanProxy(std::unique_ptr<ScanService> target)
        : target_(std::move(target))
    {
    }

    std::vector<std::string> scan(const Directory &root, Role role,
                                  const RuleExpression &rule) const override
    {
        if (root.sensitive() && role != Role::Admin)
        {
            throw std::runtime_error("权限不足：敏感目录仅管理员可扫描");
        }
        std::cout << "[审计日志] 开始扫描目录：" << root.name() << '\n';
        return target_->scan(root, role, rule);
    }

private:
    std::unique_ptr<ScanService> target_;
};
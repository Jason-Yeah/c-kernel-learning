#pragma once

#include <iostream>
#include <memory>

class Approver
{
protected:
    std::shared_ptr<Approver> nxt_;

public:
    virtual ~Approver() = default;

    std::shared_ptr<Approver> SetNext(std::shared_ptr<Approver> nxt)
    {
        nxt_ = nxt;
        return nxt_;
    }

    virtual void HandleRequest(int days) = 0;
};

class TeamLeader : public Approver
{
public:
    void HandleRequest(int days) override
    {
        if (days <= 1)
            std::cout << "  [组长] 批准 " << days << " 天假" << std::endl;
        else if (nxt_)
        {
            std::cout << "  [组长] " << days << " 天超出权限，上报经理"
                      << std::endl;
            nxt_->HandleRequest(days);
        }
        else
        {
            std::cout << "  [组长] 无人处理" << std::endl;
        }
    }
};

// ══════════════ 具体处理者 B (ConcreteHandler) ══════════════
// 模式角色：ConcreteHandler —— 经理：能批 3 天内
class Manager : public Approver
{
public:
    void HandleRequest(int days) override
    {
        if (days <= 3)
        {
            std::cout << "  [经理] 批准 " << days << " 天假" << std::endl;
        }
        else if (nxt_)
        {
            std::cout << "  [经理] " << days << " 天超出权限，上报总监"
                      << std::endl;
            nxt_->HandleRequest(days);
        }
        else
        {
            std::cout << "  [经理] 无人处理" << std::endl;
        }
    }
};

// ══════════════ 具体处理者 C (ConcreteHandler) ══════════════
// 模式角色：ConcreteHandler —— 总监：能批 7 天内
class Director : public Approver
{
public:
    void HandleRequest(int days) override
    {
        if (days <= 7)
        {
            std::cout << "  [总监] 批准 " << days << " 天假" << std::endl;
        }
        else if (nxt_)
        {
            std::cout << "  [总监] " << days << " 天超出权限，上报老板"
                      << std::endl;
            nxt_->HandleRequest(days);
        }
        else
        {
            std::cout << "  [总监] 无人处理" << std::endl;
        }
    }
};

// ══════════════ 具体处理者 D (ConcreteHandler) ══════════════
// 模式角色：ConcreteHandler —— 老板：任何假期都能批（链的终点）
class Boss : public Approver
{
public:
    void HandleRequest(int days) override
    {
        std::cout << "  [老板] 批准 " << days << " 天假" << std::endl;
        // 老板是链尾，没有 nxt_，处理完就结束
    }
};

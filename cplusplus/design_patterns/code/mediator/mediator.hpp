#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Colleague;

class Mediator
{
public:
    virtual ~Mediator() = default;
    virtual void SendMessage(const Colleague *sender,
                             const std::string &msg) = 0;
    virtual void AddColleague(std::shared_ptr<Colleague> c) = 0;
};

class Colleague
{
protected:
    std::string name_;
    Mediator *mediator_; // ← 只认识中介者，不认识其他同事

public:
    Colleague(const std::string &name, Mediator *m) : name_(name), mediator_(m)
    {
    }

    virtual ~Colleague() = default;

    virtual void Send(const std::string &msg) = 0; // 发消息（经中介者）
    virtual void Receive(const std::string &from, // 收消息（被中介者调）
                         const std::string &msg) = 0;

    const std::string &GetName() const { return name_; }
};

class ChatRoom : public Mediator
{
    std::vector<std::shared_ptr<Colleague>> members_;

public:
    void AddColleague(std::shared_ptr<Colleague> c) override
    {
        members_.push_back(std::move(c));
    }

    // ★ 转发逻辑集中在这里
    void SendMessage(const Colleague *sender, const std::string &msg) override
    {
        std::cout << "  [聊天室] " << sender->GetName() << " 说: \"" << msg
                  << "\" → 转发给所有人" << std::endl;
        for (const auto &member : members_)
        {
            if (member.get() != sender)
            { // 不给发送者自己
                member->Receive(sender->GetName(), msg);
            }
        }
    }
};

class User : public Colleague
{
public:
    User(const std::string &name, Mediator *m) : Colleague(name, m) {}

    // 发消息：不直接找别人，而是交给中介者
    void Send(const std::string &msg) override
    {
        std::cout << "[" << name_ << "] 发送: " << msg << std::endl;
        mediator_->SendMessage(this, msg); // ★ 只跟中介者打交道
    }

    // 收消息：被中介者调用
    void Receive(const std::string &from, const std::string &msg) override
    {
        std::cout << "  [" << name_ << "] 收到 " << from << ": " << msg
                  << std::endl;
    }
};

#include <iostream>
#include <stdexcept>
#include <string>

class Sender
{
public:
    virtual ~Sender() = default;
    void check(const std::string &text) const
    { // 固定的非虚规则。
        if (text.empty())
            throw std::invalid_argument("empty message");
        std::cout << "base check passed\n";
    }
    virtual void send(const std::string &text) const = 0; // 必须选择实现。
    virtual void audit() const { std::cout << "default audit\n"; }

protected:
    void defaultSend(const std::string &text) const
    {
        std::cout << "reused default send: " << text << '\n';
    }
};

class EmailSender final : public Sender
{
public:
    void send(const std::string &text) const override
    {
        std::cout << "EmailSender chooses implementation\n";
        defaultSend(text); // 明确选择复用，不是意外继承。
    }
};

int main()
{
    EmailSender email;
    const Sender &sender = email;
    
    sender.check("hello");
    sender.send("hello");
    sender.audit();
    // Sender abstract; // 编译错误：纯虚函数尚未实现。
}

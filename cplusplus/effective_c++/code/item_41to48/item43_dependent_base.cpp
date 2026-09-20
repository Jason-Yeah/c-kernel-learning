#include <iostream>
#include <string_view>

struct EmailCompany
{
    static void sendClearText(std::string_view text)
    {
        std::cout << "email: " << text << '\n';
    }
};

template <typename Company> class MessageSender
{
protected:
    void sendClear(std::string_view text) { Company::sendClearText(text); }
};

template <typename Company> class LoggingSender : public MessageSender<Company>
{
public:
    void sendWithLog(std::string_view text)
    {
        std::cout << "log before sending\n";

        // 基类依赖 Company。模板定义阶段，编译器不会自动到依赖基类中
        // 查找 sendClear；this-> 使该名称成为依赖名称，推迟到实例化时查找。
        this->sendClear(text);
    }
};

template <typename Company> class UsingSender : public MessageSender<Company>
{
private:
    // 另一种写法：先把依赖基类中的一组同名成员引入当前作用域。
    using MessageSender<Company>::sendClear;

public:
    void send(std::string_view text) { sendClear(text); }
};

int main()
{
    LoggingSender<EmailCompany> sender;
    sender.sendWithLog("build finished");

    UsingSender<EmailCompany> usingSender;
    usingSender.send("sent through a using-declaration");
}

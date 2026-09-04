#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

class Session
{
public:
    Session(std::string name, bool closeWillFail)
        : name_(std::move(name)), closeWillFail_(closeWillFail)
    {
    }

    void close()
    {
        if (closed_)
            return;
        if (closeWillFail_)
            throw std::runtime_error("close failed: " + name_);

        closed_ = true;
        std::cout << "closed " << name_ << '\n';
    }

    ~Session() noexcept
    {
        if (closed_)
            return;

        try
        {
            close();
        }
        catch (const std::exception &error)
        {
            // 析构函数中只能记录并兜底，绝不让异常逃出。
            std::cerr << "cleanup warning: " << error.what() << '\n';
        }
    }

private:
    std::string name_;
    bool closeWillFail_{};
    bool closed_{};
};

int main()
{
    Session normal{"normal-session", false};
    normal.close(); // 需要可靠报告错误的操作，应显式调用并在此处处理

    Session failing{"failing-session", true};
    try
    {
        failing.close();
    }
    catch (const std::exception &error)
    {
        std::cerr << "explicit close reports: " << error.what() << '\n';
    }
} // failing 的析构函数仍会尝试清理，但只记录异常，不会终止程序

#include <iostream>
#include <stdexcept>

class Account
{
public:
    explicit Account(long long initialCents) : balanceCents_(initialCents)
    {
        if (initialCents < 0)
            throw std::invalid_argument("initial balance cannot be negative");
    }

    void deposit(long long cents)
    {
        if (cents <= 0)
            throw std::invalid_argument("deposit must be positive");
        balanceCents_ += cents;
    }

    bool withdraw(long long cents)
    {
        if (cents <= 0 || cents > balanceCents_)
            return false;
        balanceCents_ -= cents;
        return true;
    }

    long long balanceCents() const noexcept { return balanceCents_; }

private:
    long long balanceCents_{};
};

int main()
{
    Account account{1000};
    account.deposit(500);
    const bool paid = account.withdraw(1200);
    std::cout << "paid=" << std::boolalpha << paid
              << ", balance=" << account.balanceCents() << '\n';

    // account.balanceCents_ = -1; // 编译错误：外部无法绕过 Account 的规则
}

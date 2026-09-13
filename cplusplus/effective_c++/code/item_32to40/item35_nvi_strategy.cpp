#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>

class Character
{
public:
    virtual ~Character() = default;
    int health() const
    { // NVI：固定入口统一校验。
        const int value = doHealth();
        if (value < 0)
            throw std::logic_error("negative health");
        return value;
    }

private:
    virtual int doHealth() const = 0;
};

class FixedCharacter final : public Character
{
public:
    explicit FixedCharacter(int value) : value_(value) {}

private:
    int doHealth() const override
    {
        return value_;
    } // 可以覆盖基类 private 虚函数。
    int value_;
};

class StrategyCharacter
{
public:
    using Algorithm = std::function<int(int)>;
    StrategyCharacter(int base, Algorithm algorithm)
        : base_(base), algorithm_(std::move(algorithm))
    {
        if (!algorithm_)
            throw std::invalid_argument("empty strategy");
    }

    int health() const
    {
        const int value = algorithm_(base_);
        if (value < 0)
            throw std::logic_error("negative health");
        return value;
    }

private:
    int base_;
    Algorithm algorithm_;
};

int main()
{
    FixedCharacter good{80};
    std::cout << "NVI health: " << good.health() << '\n';
    bool rejected = false;
    try
    {
        FixedCharacter bad{-1};
        (void)bad.health();
    }
    catch (const std::logic_error &e)
    {
        rejected = true;
        std::cout << "NVI rejected: " << e.what() << '\n';
    }

    assert(rejected);
    StrategyCharacter normal{100, [](int base) { return base; }};
    const int damage = 25;
    StrategyCharacter injured{100,
                              [damage](int base) { return base - damage; }};
    std::cout << "strategies: " << normal.health() << ", " << injured.health()
              << '\n';
    assert(normal.health() == 100 && injured.health() == 75);
}

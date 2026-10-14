#include <iostream>
#include <string>

// C++11 没有语言级协程。把执行位置和跨暂停点的数据手工保存到对象。
class Conversation
{
    enum class State
    {
        first,
        second,
        done
    };
    State state_ = State::first;
    std::string name_;
    int value_ = 0;

public:
    explicit Conversation(std::string name) : name_(name) {}

    bool done() const { return state_ == State::done; }

    void resume()
    {
        switch (state_)
        {
        case State::first:
            value_ = 10;
            std::cout << name_ << ": first, value=" << value_ << '\n';
            state_ = State::second;
            return;
        case State::second:
            ++value_;
            std::cout << name_ << ": second, value=" << value_ << '\n';
            state_ = State::done;
            return;

        case State::done:
            return;
        }
    }
};

int main()
{
    Conversation a("A"), b("B");
    while (!a.done() || !b.done())
    {
        a.resume();
        b.resume();
    }
}

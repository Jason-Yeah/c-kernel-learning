#include <string>

class TextBlock
{
public:
    const char &operator[](std::size_t pos) const { return text_[pos]; }

    char &operator[](std::size_t pos)
    {
        return const_cast<char &>(static_cast<const TextBlock &>(*this)[pos]);
    }

    // *this == TextBlock&
    // static_cast<const TextBlock &>(*this)
    //      表示把*this当作const TextBlock&来看待
    //      重载成第一版的[]得到const char&
    // const_cast<char&>(...) 解除const 变成non-const

private:
    std::string text_;
};

class CLAZZ
{
    std::string str;
    int x;

public:
    CLAZZ() : str(), x(0) {}
};

int main() {}

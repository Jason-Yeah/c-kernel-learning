#include <iostream>
#include <string_view>

// c17 __cplusplus >= 201703L
// c20 __cplusplus >= 202002L
//
#if __cplusplus >= 202002L
#include <concepts>

template <typename T>
concept DrawableLike = requires(const T &object) {
    { object.draw() } -> std::same_as<void>;
};

template <DrawableLike T> void renderTwice(const T &object)
#else
template <typename T> void renderTwice(const T &object)
#endif
{
    // 模板没有要求 T 继承某个基类，只要求这个表达式能够通过编译。
    object.draw();
    object.draw();
}

class Circle
{
public:
    void draw() const { std::cout << "draw a circle\n"; }
};

class Button
{
public:
    explicit Button(std::string_view text) : text_(text) {}

    void draw() const { std::cout << "draw button: " << text_ << '\n'; }

private:
    std::string_view text_;
};

int main()
{
    Circle circle;
    Button button{"Save"};

    renderTwice(circle);
    renderTwice(button);
}

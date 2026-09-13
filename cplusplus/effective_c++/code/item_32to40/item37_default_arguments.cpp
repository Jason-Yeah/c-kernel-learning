#include <iostream>

enum class Color
{
    red,
    blue
};
const char *name(Color c) { return c == Color::red ? "red" : "blue"; }

struct Shape
{
    virtual ~Shape() = default;
    virtual void draw(Color color = Color::red) const = 0;
};

struct Circle final : Shape
{
    void draw(Color color = Color::blue) const override
    {
        std::cout << "Circle body, color=" << name(color) << '\n';
    }
};

class SafeShape
{
public:
    virtual ~SafeShape() = default;
    void draw(Color color = Color::red) const { doDraw(color); }

private:
    virtual void doDraw(Color color) const = 0;
};

class SafeCircle final : public SafeShape
{
private:
    void doDraw(Color color) const override
    {
        std::cout << "NVI Circle body, color=" << name(color) << '\n';
    }
};
int main()
{
    Circle c;
    const Shape &base = c;

    c.draw();    // Circle 默认 blue。
    base.draw(); // Shape 默认 red，但仍调用 Circle 函数体。

    SafeCircle safe;
    const SafeShape &safeBase = safe;

    safe.draw();
    safeBase.draw(); // 两者使用同一个公开入口的默认值。
}

#include <cassert>
#include <iostream>

// Rectangle 契约：改宽不改变高。
class Rectangle
{
public:
    Rectangle(int w, int h) : width_(w), height_(h) {}
    virtual ~Rectangle() = default;
    virtual void setWidth(int w) { width_ = w; }
    int height() const { return height_; }

protected:
    int width_;
    int height_;
};

class Square final : public Rectangle
{
public:
    explicit Square(int side) : Rectangle(side, side) {}
    void setWidth(int w) override { width_ = height_ = w; }
};

bool checkContract(Rectangle &r)
{
    const int previous = r.height();
    r.setWidth(10);
    return r.height() == previous;
}

int main()
{
    Rectangle rectangle{3, 4};
    Square square{4};
    const bool regular = checkContract(rectangle);
    const bool derived = checkContract(square);
    std::cout << std::boolalpha << "Rectangle preserves height: " << regular
              << '\n'
              << "Square preserves height: " << derived << '\n';
    assert(regular && !derived); // 可运行的契约反例，不是未定义行为。
}

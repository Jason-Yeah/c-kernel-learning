#include <iostream>
#include <utility>

class BitmapHolder
{
public:
    explicit BitmapHolder(int pixel) : pixel_(new int(pixel)) {}

    BitmapHolder(const BitmapHolder &rhs) : pixel_(new int(*rhs.pixel_)) {}

    friend void swap(BitmapHolder &left, BitmapHolder &right) noexcept
    {
        std::swap(left.pixel_, right.pixel_);
    }

    BitmapHolder &operator=(BitmapHolder rhs)
    {
        // rhs 在进入函数前已成功复制（或移动）；交换后 rhs 持有旧资源。
        swap(*this, rhs);
        return *this;
    }

    ~BitmapHolder() { delete pixel_; }

    int value() const { return *pixel_; }

private:
    int *pixel_{};
};

int main()
{
    BitmapHolder holder{10};
    holder = holder; // copy-and-swap 先复制，再交换；不会读取已释放资源
    std::cout << "after self assignment: " << holder.value() << '\n';

    BitmapHolder other{20};
    holder = other;
    std::cout << "after normal assignment: " << holder.value() << '\n';
}

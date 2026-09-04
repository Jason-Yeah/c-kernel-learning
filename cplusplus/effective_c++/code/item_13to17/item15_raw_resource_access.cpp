#include <iostream>
#include <memory>

struct NativeImage
{
    explicit NativeImage(int id) : id(id) {}
    int id{};
};

// 模拟旧 API：它只临时使用指针，不会 delete。
void legacyDraw(const NativeImage *image)
{
    std::cout << "legacy API draws image " << image->id << '\n';
}

class Image
{
public:
    explicit Image(int id) : image_(std::make_unique<NativeImage>(id)) {}

    NativeImage *get() const noexcept { return image_.get(); }

    std::unique_ptr<NativeImage> release() noexcept
    {
        return std::move(image_); // 明确把所有权交给调用者
    }

private:
    std::unique_ptr<NativeImage> image_;
};

int main()
{
    Image image{7};
    NativeImage *borrowed = image.get();
    legacyDraw(borrowed); // 只借用；image 仍拥有对象

    // delete borrowed; // 错误：image 析构时还会再次删除同一对象,
    /*
        new owner has image -446226326
        free(): double free detected in tcache 2
        zsh: IOT instruction (core dumped)  ./a.out
    */

    std::unique_ptr<NativeImage> newOwner = image.release();
    std::cout << "new owner has image " << newOwner->id << '\n';

    return 0;
} // newOwner 析构时删除 NativeImage；image 此时为空

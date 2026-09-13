#include <iostream>
#include <type_traits>

class Base {
public:
    virtual ~Base() = default;
    virtual const char *name() const { return "Base"; }
};

class Derived : public Base {
public:
    const char *name() const override { return "Derived"; }
};

template <typename T>
class PtrView {
public:
    explicit PtrView(T *pointer) : pointer_(pointer) {}

    // 这是成员函数模板。只有 U* 能隐式转换成 T* 时，这个构造函数才参与匹配。
    template <typename U,
              typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    PtrView(const PtrView<U> &other) : pointer_(other.get())
    {
    }

    T *get() const { return pointer_; }
    T &operator*() const { return *pointer_; }
    T *operator->() const { return pointer_; }

private:
    T *pointer_;
};

static_assert(std::is_constructible_v<PtrView<Base>, PtrView<Derived>>);
static_assert(!std::is_constructible_v<PtrView<Derived>, PtrView<Base>>);

int main()
{
    Derived derived;
    PtrView<Derived> derivedView{&derived};

    // 模拟 shared_ptr<Derived> 可以转成 shared_ptr<Base> 的方向。
    PtrView<Base> baseView = derivedView;
    std::cout << baseView->name() << '\n';
}

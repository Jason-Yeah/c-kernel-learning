#include <cstddef>
#include <iostream>

// ============================================================
// 改造前：大算法直接放在模板中
// ============================================================

namespace Before
{

template <typename T> class PointerBox
{
public:
    __attribute__((noinline)) void push(T *p)
    {
        if (count_ >= 4)
            return;

        // 假装这里是一段比较大的公共算法
        for (std::size_t i = count_; i > 0; --i)
        {
            slots_[i] = slots_[i - 1];
        }

        slots_[0] = p;
        ++count_;
    }

private:
    T *slots_[4]{};
    std::size_t count_{0};
};

} // namespace Before

// ============================================================
// 改造后：大算法抽到非模板基类中
// ============================================================

namespace After
{

class PointerBoxBase
{
protected:
    __attribute__((noinline)) void push_impl(void *p)
    {
        if (count_ >= 4)
            return;

        // 真正的大算法只在这里存在一份
        for (std::size_t i = count_; i > 0; --i)
        {
            slots_[i] = slots_[i - 1];
        }

        slots_[0] = p;
        ++count_;
    }

private:
    void *slots_[4]{};
    std::size_t count_{0};
};

template <typename T> class PointerBox : private PointerBoxBase
{
public:
    __attribute__((noinline)) void push(T *p)
    {
        // 模板里只留下一个很薄的包装
        this->push_impl(static_cast<void *>(p));
    }
};

} // namespace After

int main()
{
    int i = 10;
    double d = 3.14;

    Before::PointerBox<int> beforeInt;
    Before::PointerBox<double> beforeDouble;

    beforeInt.push(&i);
    beforeDouble.push(&d);

    After::PointerBox<int> afterInt;
    After::PointerBox<double> afterDouble;

    afterInt.push(&i);
    afterDouble.push(&d);

    std::cout << "done\n";
}

/*
Before: 每个对象编译器都生成了大代码，本例是两份
        L + L
After: 每个对象编译器生成少量代码用于跳转到同一个大代码中
        s + s + L
*/

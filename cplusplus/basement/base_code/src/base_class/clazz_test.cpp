#include <iostream>
#include <cstring>

class Example
{
    public:
        Example() : data_(0)
        {
            std::cout << "Default constructor called.\n";
        }

        Example(int data) : data_(data)
        {
            std::cout << "Parameterized constructor called with data = " << data_ << ".\n";
        }

        // 拷贝构造函数
        Example(const Example& other) : data_(other.data_)
        {
            std::cout << "Copy constructor called.\n";
        }

        // 移动构造函数（右值）
        Example(Example&& other) noexcept : data_(other.data_)
        {
            other.data_ = 0;
            std::cout << "Move constructor called.\n";
        }

        // 析构函数
        ~Example() 
        {
            std::cout << "Destructor called for data = " << data_ << ".\n";
        }

    private:
        int data_;
};

class MyString
{
    private:
        char* data_;
        // #define __SIZE_TYPE__ long unsigned int
        // typedef __SIZE_TYPE__ 	size_t;
        // 跨平台都是8B（通过__SIZE_TYPE_在GCC/CLANG编译器启动时塞进去个内置宏）
        std::size_t size_;
    public:
        MyString(const char* str = nullptr)
        {
            if (str)
            {
                size_ = std::strlen(str);
                data_ = new char[size_ + 1]; // '\0'
                std::strcpy(data_, str); // 适合搬运'\0'这种字符
            }
            else
            {
                size_ = 0;
                data_ = new char[1];
                data_[0] = '\0';
            }
            std::cout << "Constructor called.\n";
        }

        MyString(const MyString& other) : size_(other.size_)
        {
            data_ = new char[size_];
            std::strcpy(this->data_, other.data_);
            std::cout << "Copy constructor called.\n";
        }

        MyString& operator = (const MyString& other)
        {
            std::cout << "Copy assignment operator called.\n";
            if (this == &other)
                return *this;
            
                delete[] data_;

                size_ = other.size_;
                data_ = new char[size_ + 1];
                std::strcpy(data_, other.data_);
                return *this;
        }

        ~MyString()
        {
            delete[] data_;
            std::cout << "Destructor called.\n";
        }

        void print_str() const
        {
            std::cout << data_ << "\n";
        }
};

class Box
{
    private:
        double length_;
        double width_;
        double height_;

    public:
        Box(double length, double width, double height) 
        : length_(length), width_(width), height_(height) {}

        friend double calc_v(const Box& b);
};

double calc_v(const Box& b)
{
    return b.height_ * b.width_ * b.height_;
}

class Rectangle
{
    private:
        double width_;
        double height_;

        friend class AreaCalculator;
    public:
        Rectangle(double width, double height) : width_(width), height_(height) {}
};

class AreaCalculator
{
    public:
        double calcu(const Rectangle& rect)
        {
            return rect.height_ * rect.width_;
        }
};

int main()
{
    Example e1;
    Example e2(10);
    Example e3 = e2;
    Example e4 = std::move(e2);
    Example e5 = Example(10);

    MyString s1("Jason");
    MyString s2 = s1;
    MyString s3("fk");
    s3 = s1;

    s1.print_str();
    s2.print_str();
    s3.print_str();

    return 0;
}
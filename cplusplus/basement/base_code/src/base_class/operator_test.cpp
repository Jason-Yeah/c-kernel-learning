#include <iostream>
#include <string>

class Point
{
    private:
        double x_;
        double y_;
    public:
        Point() : x_(0), y_(0) {}

        Point(double x, double y) : x_(x), y_(y) {}

        Point operator + (const Point& other) const
        {
            return Point(this->x_ + other.x_, this->y_ + other.y_);
        }

        void print() const 
        {
            std::cout << "(" << x_ << ", " << y_ << ")\n";
        }

        friend Point operator - (const Point& a, const Point& b);
        friend void set_info(Point& a, int x, int y);
        friend std::ostream& operator << (std::ostream& os, const Point& point);
};

Point operator-(const Point& a, const Point& b) 
{
    return Point(a.x_ - b.x_, a.y_ - b.y_);
}

void set_info(Point& a, int x, int y)
{
    a.x_ = x; 
    a.y_ = y; 
}

std::ostream& operator << (std::ostream& os, const Point& point)
{
    os << "(" << point.x_ << ", " << point.y_ << ")";
    return os;
}

class Animal
{
    public:
        void eat() const 
        {
            std::cout << "Animal eats." << std::endl;
        }

        virtual void speak() 
        {
            std::cout << "动物发出叫声" << std::endl;
        }
    protected:
        int age_;
};

class Dog : public Animal
{
    public:
        void bark() const {
            std::cout << "Dog barks." << std::endl;
        }

        void setAge(int a) {
            age_ = a; // 访问受保护成员
        }

        int getAge() const {
            return age_;
        }

        // override重载父类函数
        void speak() override 
        {
            std::cout << "汪汪汪！" << std::endl;
        }
};

int main()
{
    Point p1(1.5, 2.5);
    Point p2(3.0, 4.0);

    Point p3 = p1 + p2;
    p3.print(); // 输出： (4.5, 6.5)

    Point p4 = p2 - p1;
    p4.print(); // 输出： (1.5, 1.5)

    Dog myDog;
    myDog.eat();    // 继承自Animal
    myDog.bark();   // Dog特有

    myDog.setAge(5);
    std::cout << "Dog's age: " << myDog.getAge() << std::endl;

    Animal* ptr = &myDog;
    ptr->speak();

    set_info(p1, 10, 20);
    p1.print(); // 输出： (10, 20)
    std::cout << p2 << std::endl; // 输出： (10, 20)

    return 0;
}
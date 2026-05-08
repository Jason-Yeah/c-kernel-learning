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

        friend Point operator-(const Point& a, const Point& b);
};

Point operator-(const Point& a, const Point& b) 
{
    return Point(a.x_ - b.x_, a.y_ - b.y_);
}

int main()
{
    Point p1(1.5, 2.5);
    Point p2(3.0, 4.0);

    Point p3 = p1 + p2;
    p3.print(); // 输出： (4.5, 6.5)

    Point p4 = p2 - p1;
    p4.print(); // 输出： (1.5, 1.5)

    return 0;
}
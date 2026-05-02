#include <iostream>

struct Stu
{
    int id;
    std::string name;
    float grade;

    Stu(int s_id, std::string s_name, float s_grade) : 
        id(s_id), name(s_name) 
        {
            grade = s_grade;
        }
};

struct Point {
    int x;
    int y;
};

void print_p(Point p)
{
    printf("%d %d\n", p.x, p.y);
}

void move_p(Point &p, int dx, int dy)
{
    p.x += dx;
    p.y += dy;
}

void move_p2(Point *p, int dx, int dy)
{
    p->x += dx;
    p->y += dy;
}

class Rectangle {
private:
    int width;
    int height;

public:
    void setDimensions(int w, int h) {
        width = w;
        height = h;
    }

    int area() const {
        return width * height;
    }

    int get_width()
    {
        return width;
    }
};

typedef struct {
    int id;
    std::string name;
    float grade;
} Student;

// 或者使用 `using`（C++11 及以上）
using Student2 = struct {
    int id;
    std::string name;
    float grade;
};

int main()
{
    Stu stu1(100, "jason", 95.0f);

    Rectangle rec;
    rec.setDimensions(1, 2);
    auto width = rec.get_width();

    std::cout << "rec_w: " << width << std::endl;

    Point p = {1, 2};
    print_p(p);
    move_p2(&p, 2, 3);
    print_p(p);

    return 0;
}
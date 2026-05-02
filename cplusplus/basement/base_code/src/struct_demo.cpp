#include <iostream>
#include <string>
#include <iomanip>
#include <vector>

struct Student
{
    int id;
    std::string name;
    double grade;

    void set_info(const int sid, const std::string sname, const double sgrade)
    {
        id = sid;
        name = sname;
        grade = sgrade;
    }

    void print_info()
    {
        std::cout << id << " " 
              << name << " " 
              << std::fixed << std::setprecision(2) << grade << std::endl;
    }
};

int main()
{
    std::vector<Student> vecs;

    int id;
    std::string name;
    double grade;
    std::cin >> id >> name >> grade;

    Student stu;
    stu.set_info(id, name, grade);
    vecs.push_back(stu);

    for (auto stu_t: vecs)
    {
        stu_t.print_info();
    }

    return 0;
}
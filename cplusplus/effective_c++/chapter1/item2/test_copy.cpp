#include <iostream>
#include <string>

class Stu
{
    public:
        std::string name;

        Stu(std::string n) : name(n)
        {
            std::cout << "The constructor of Stu is called with name = " << name << std::endl;
        }

        Stu(const Stu& other)
        {
            this->name = other.name;
            std::cout << "The copy constructor of Stu is called with name = " << name << std::endl;
        }

        Stu& operator=(const Stu& other)
        {
            std::cout << "The assignment operator of Stu is called with name = " << other.name << std::endl;
            if (this != &other)
            {
                this->name = other.name;
            }
            return *this;
        }
};

int main()
{
    Stu s1("Jason");

    Stu s2 = s1;

    Stu s3("David");
    s3 = s1;

    return 0;
}

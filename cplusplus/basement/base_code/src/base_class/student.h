#include <string>

class Stu
{
    private:
        std::string name_;
        int age_;

    public:
        Stu(const std::string& name, int age);

        void set_name(const std::string& name);
        
        std::string get_name() const;

        void set_age(int age);

        int get_age() const;
};

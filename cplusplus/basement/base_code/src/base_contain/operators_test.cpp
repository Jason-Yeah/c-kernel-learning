#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// callback_func
typedef void (*CallBack)();

using callback = void(*)();

// 给 "返回void且无参数的函数指针" 起个名字叫 FuncPtr
using funcptr = void(*)();
class Person
{
    public:
        std::string name;
        int age;
        void sayhi()
        {
            std::cout << "Hi, I'm " << name << std::endl;
        }
        bool operator < (const Person& other) const{
            return this->age < other.age;
        }
};

inline double div(double a, double b)
{
    if (b == 0) throw std::invalid_argument("Denominator cannot be zero."); // 抛出异常
    return a / b;
}

void execute_f(funcptr func_p)
{
    func_p();
}

void greet()
{
    printf("...............\n");
}

void register_callback(callback cb)
{
    std::cout << "before callback" << std::endl;
    cb();
    std::cout << "After callback" << std::endl;
}

// 回调函数
void myCallback() {
    std::cout << "Callback executed!" << std::endl;
}

int main()
{
    int Person::* prt_age = &Person::age;
    Person person1 = {"Jason", 23};
    Person person2 = {"Bob", 30};

    void (Person::*ptr_func)() = &Person::sayhi;

    std::cout << person1.age << std::endl;
    std::cout << person1.*prt_age << std::endl;
    std::cout << person2.*prt_age << std::endl;

    std::cout << (person2 < person1) << std::endl;

    (person1.*ptr_func)();

    // ==========
    
    // {
    //     int n = 0;
    //     std::cout << "n: ";
    //     std::cin >> n;
    //     if (n > 8) goto end;
    //     std::cout << "fku" << std::endl;

    //     end:
    //         std::cout << "n > 8, yeah." << std::endl;
    // }

    double a, b;
    std::cout << "Enter: ";
    std::cin >> a >> b;

    try {
        double result = div(a, b);
        std::cout << "Result: " << result << std::endl;
    } catch (std::invalid_argument &e) { // 捕获 std::invalid_argument 异常
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // ====================

    std::vector<int> nums = {1, 3, 2, 5, 4};

    std::for_each(nums.begin(), nums.end(), [](int x){
        std::cout << x << " ";
    });
    puts("");

    std::sort(nums.begin(), nums.end(), [](int a, int b) -> bool {
        return a > b;
    });

    std::for_each(nums.begin(), nums.end(), [](int x){
        std::cout << x << " ";
    });
    puts("");

    // =====================================

    void (*func_greet_ptr)() = greet;

    func_greet_ptr();
    execute_f(func_greet_ptr);

    register_callback(myCallback);

    {
        callback cb = []() -> void {
            std::cout << "Lambda callback!" << std::endl;
        };
        execute_f(cb);
        execute_f([]() -> void {
            std::cout << "Lambda callback!" << std::endl;
        });
    }

    return 0;
}
#include "ref_demo.h"

int main()
{
    int a = 10;
    int& b = a;
    int&& c = 20;
    int d = a + 5;
    std::string s1 = "J";
    std::string s2 = std::string("Y");

    std::cout << std::boolalpha;
    std::cout << "a is letfvr: " << std::is_lvalue_reference_v<decltype(a)> << std::endl;
    std::cout << "(a) is letfvr: " << std::is_lvalue_reference_v<decltype((a))> << std::endl;
    std::cout << "b is letfvr: " << std::is_lvalue_reference_v<decltype(b)> << std::endl;
    std::cout << "c is rvr: " << std::is_rvalue_reference_v<decltype(c)> << std::endl;

    func(a);
    // func(c);
    func(10);

    check_ref(a);
    check_ref(10);
    puts("========");
    check_ref(c);
    puts("========");
    check_ref(std::move(c)); // 将亡值是右值，是int，T是int, x是int&&
    puts("========");
    check_ref(std::forward<int&&>(c));

    wrapper(a);
    wrapper(10);

    int da = 5;
    const int& ref = da;
    deduce_type(da);
    deduce_type(ref);

    display_type(ref);
    display_type(da);

    double db = 0.5;
    display_type(db);

    use_ftmp();
    use_ftmp2();

    return 0;
}
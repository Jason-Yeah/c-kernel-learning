#include <iostream>

class B 
{
    public:
        B()
        {
            std::cout << "The default constructor of B is called" << std::endl;
        }
        // B(int x = 0)
        // {
        //     std::cout<< "The constructor of B is called with x = " << x << std::endl;
        // }
    /*
        explicit B(int x)
        {
            std::cout<< "The constructor of B is called with x = " << x << std::endl;
        }
    */
};

void func(B b)
{
    std::cout<< "Function func is called" << std::endl;
}

int main()
{
    // func(B(28));
    B b;
    //func(28);
    
    return 0;
}
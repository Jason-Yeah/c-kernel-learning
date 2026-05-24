#include <iostream>

class MyClass {
public:
    int secret;
    int id;

    // 非静态函数：依赖 this
    void setSecret(int val) {
        secret = val; 
    }

    // 静态函数：不依赖 this
    static void printHello() {
        std::cout << "Hello from Static!" << std::endl;
    }
};

int main() {
    MyClass obj;
    obj.id = 100;
    
    // 设置断点后，我们将单步进入这个函数
    obj.setSecret(42); 
    
    MyClass::printHello();

    return 0;
}
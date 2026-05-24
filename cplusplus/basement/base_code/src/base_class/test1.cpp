#include <iostream>

typedef void (*func)(void *);

class Base {
public:
    int a;

    virtual void funcA() { std::cout << "Base::funcA\n"; } // Index 0
    virtual void funcB() { std::cout << "Base::funcB\n"; } // Index 1
};

int main() {
    Base obj;
    obj.a = 100;

    std::cout << sizeof(obj) << std::endl;
    // 1. 获取对象起始地址 (即 vptr 的地址)
    size_t* p = (size_t*)&obj;

    // 2. 取出 vptr (对象前8字节的内容)
    size_t* vtable = (size_t*)*p;

    // 3. 取出第 0 个虚函数的地址 (funcA)
    func fp = (func)vtable[0];

    // 4. 调用！注意：必须手动传入 &obj 作为 this 指针
    fp(&obj); 

    fp = (func)vtable[1];

    // 4. 调用！注意：必须手动传入 &obj 作为 this 指针
    fp(&obj); 

    char* obj_bytes = (char*)&obj;
    size_t vtbl_size = sizeof(void *);
    int* var_p = (int*)(obj_bytes + vtbl_size);


    std::cout << (int *)obj_bytes << std::endl;
    std::cout << var_p << std::endl;
    std::cout << *var_p << std::endl;

    return 0;
}
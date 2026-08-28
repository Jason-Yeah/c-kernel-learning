// singleton_guard.cpp
// 编译命令: g++ -g -O0 -std=c++11 -o singleton_guard singleton_guard.cpp
// 注意: 必须用 -O0 关闭优化，否则编译器可能内联或优化掉部分guard逻辑

#include <cstdio>

struct Singleton {
    int value;
    
    Singleton() : value(42) {
        printf("[Constructor] Singleton created, value=%d\n", value);
    }
    
    ~Singleton() {
        printf("[Destructor] Singleton destroyed\n");
    }
};

// Magic Static 单例
Singleton& GetInstance() {
    static Singleton instance; // 编译器在此处插入 guard 变量和 __cxa_guard_* 调用
    return instance;
}

int main() {
    printf("=== Before first call ===\n");
    
    Singleton& s1 = GetInstance();
    printf("s1.value = %d, addr = %p\n", s1.value, (void*)&s1);
    
    printf("=== Before second call ===\n");
    
    Singleton& s2 = GetInstance();
    printf("s2.value = %d, addr = %p\n", s2.value, (void*)&s2);
    
    printf("=== Same object? %s ===\n", (&s1 == &s2) ? "YES" : "NO");
    
    return 0;
}

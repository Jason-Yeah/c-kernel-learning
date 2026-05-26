#include <iostream>
#include <memory>
#include <cstring>

struct Foo {
    char data[64];
};

int main() 
{
    auto p = std::make_shared<Foo>();
    auto raw = (char*)p.get();

    std::cout << "Foo 对象地址: "     << (void*)raw          << "\n";
    std::cout << "Foo 前 8 字节: "    << (void*)(raw - 8)    << "\n";
    std::cout << "Foo 前 16 字节: "   << (void*)(raw - 16)   << "\n";
    std::cout << "Foo 前 32 字节: "   << (void*)(raw - 32)   << "\n\n";

    // 读控制块区域的 vtable 指针，确认控制块的起始位置
    // vtable 指向 .rodata 段（代码区的地址），不会是 0
    void* vtable = nullptr;
    std::memcpy(&vtable, raw - 16, 8);
    std::cout << "Foo 前 16 字节处的 vtable: " << vtable << "\n";

    void* maybe_vtable2 = nullptr;
    std::memcpy(&maybe_vtable2, raw - 24, 8);
    std::cout << "Foo 前 24 字节处的值: " << maybe_vtable2 << "\n";

    void* maybe_vtable3 = nullptr;
    std::memcpy(&maybe_vtable3, raw - 32, 8);
    std::cout << "Foo 前 32 字节处的值: " << maybe_vtable3 << "\n\n";

    // 判断控制块从哪里开始 —— vtable 地址指向代码段（0x5xxxxx 或 0x4xxxxx）
    // 不像 0 或 0x61 这类明显不是指针的值
    printf("分析: 控制块的 vtable 在 Foo 前 16 字节处\n");
    printf("      vtable(8B) + refcount(4B) + weakcount(4B) = 16 字节\n");
    printf("      控制块大小 ≈ 16 字节（不含 Foo 本身）\n\n");

    // 对比: new + shared_ptr 和 make_shared 的分配次数
    std::cout << "─── 对比测试 ───\n";
    std::cout << "make_shared<Foo> 的地址:\n";
    std::cout << "  Foo 对象:  " << (void*)raw << "\n";
    // 控制块就在 Foo 前面，和对象在同一块内存

    std::shared_ptr<Foo> p2(new Foo());
    auto raw2 = (char*)p2.get();
    std::cout << "new Foo + shared_ptr 的地址:\n";
    std::cout << "  Foo 对象:  " << (void*)raw2 << "\n";
    // 用 new 时，控制块在另一个单独分配的内存块里
    // 两次分配，两处地址没有固定关系

    return 0;
}

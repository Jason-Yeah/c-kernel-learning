#include <cstdio>
#include <new>

struct Trivial { int x; };           // 无析构 → 平凡
struct NonTrivial { ~NonTrivial(){} }; // 有析构 → 非平凡

int main() {
    // 分配并打印返回地址前后的内存
    auto* t = new Trivial[100];
    auto* n = new NonTrivial[4];

    printf("Trivial   ptr: %p\n", (void*)t);
    printf("  前8字节(cookie): %zu\n", *((size_t*)t - 1));

    printf("NonTrivial ptr: %p\n", (void*)n);
    printf("  前8字节(cookie): %zu\n", *((size_t*)n - 1));

    delete[] t;
    delete[] n;
}
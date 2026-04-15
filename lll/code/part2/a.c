int x;          // 可能形成 COMMON
int y = 10;     // 一定在 .data
int z;          // 可能形成 COMMON

extern int w;   // 只是声明，不分配空间

int main() {
    return x + y + z + w;
}
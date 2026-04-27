# ELF补

## extern "C"

一个关键的链接指示符，告诉C++ 编译器，按照 C 语言的方式来编译和链接代码。

C++ 支持函数重载（Overloading），而 C 语言不支持。为了区分同名但参数不同的函数，C++ 编译器会进行 符号修饰（Name Mangling）。

假设你有一个函数：`void print(int i);`

- C 编译器：生成的符号名通常就是 print。
- C++ 编译器：生成的符号名可能是 _Z5printi（其中 i 代表 int）。

**问题**：如果你用 C++ 调用一个现成的 C 语言库（比如标准输入输出库），C++ 链接器会去寻找 _Z5printi，但 C 库里只有 print。结果就是：链接失败（LNK2019 / Undefined Reference）。

加上 extern "C" 时，你是在强制要求 C++ 编译器：“收起你那套复杂的修饰规则，老老实实生成 C 风格的符号名。”

格式

```cpp
extern "C" void print_s(char* str);

extern "C"
{
    void func1();
    void func2(int a);
}
```

常用方式：

```cpp
#ifndef MY_PRINT_H
#define MY_PRINT_H

#ifdef __cplusplus
extern "C" 
{
#endif

void print_s(char* str);

#ifdef __cplusplus
}
#endif

#endif
```

这里`__cplusplus`是C++的宏，C++编译器会在编译C++程序时默认定义该宏，如果是CPP环境就加入extern "C"，如果是C，正好不用加了。

## 强弱符号

对C/C++，编译器默认函数和初始化的全局变量是强符号，未初始化的全局变量是弱符号。也可通过GCC的"__attribute__((weak))"定义任意一个强符号为弱符号。（仅针对定义，外部引用啥也不是）

RULES：

1. 强符号不可被多次定义
2. 如果一个符号在某个定义是强符号，在其他文件都是弱符号，选强符号
3. 一个符号在所有文件都是弱符号，选占用空间最大的一个。

强引用：只要强引用还在，内存就不会被释放。它会增加“引用计数”（容易产生循环引用（A 指向 B，B 指向 A），导致内存泄漏，谁都释放不了。）

弱引用：不会增加引用计数。用来监控强引用。当你想知道某个对象是否还活着，但又不想干扰它的生命周期时，就用弱引用。（该符号未被定义，则链接器对于该引用不报错）

如果一个程序被设计成可以支持单线程或多线、程的模式，就可以通过弱引用的方法来判断当前的程序是链接到了单线程的Glibc库还是多线程的Glibc库（是否在编译时有-lpthread选项），从而执行单线程版本的程序或多线程版本的程序。(如果你使用的是较新版本的 Ubuntu（比如 22.04 或 24.04），libpthread 已经被整合进 libc.so 了)

# 静态链接

## 空间与地址分配

常用的链接合并文件方式是相似段合并，也就是.text放一块，.data放一块这样。

链接器一般采用两步链接：一、空间与地址分配；二、符号解析与重定位

1. 扫描所有输入目标文件，获得各自段/节的长度属性和位置，把符号表中所有符号定义和符号引用收集起来，放到一个全局符号表中。
2. 符号解析与重定位，使用上述收集到的信息，读取输入文件中的段/节数据、重定位信息，进行符号解析与重定位，跳转代码中的地址。

## ELF文件装载入内存布局

首先明确ELF本身文件里存的是各种表和Section，装载到内存后视角变成了Segment（通过Program Header Table）。

从进程角度装载入内存布局大致为：

```text
进程中主程序映像（PIE 基址约 0x555555554000）

0x555555554000 ─────────────────────────────────────
                 [只读元信息映射段  R--]
                 ELF Header
                 Program Header Table
                 .interp
                 .note.gnu.property
                 .note.gnu.build-id
                 .note.ABI-tag
                 .gnu.hash
                 .dynsym
                 .dynstr
                 .gnu.version
                 .gnu.version_r
                 .rela.dyn
                 .rela.plt

0x555555555000 ─────────────────────────────────────
                 [代码映射段  R-X]
                 .init
                 .plt
                 .plt.got
                 .plt.sec
                 .text
                   ├─ _start
                   ├─ add
                   └─ main
                 .fini

0x555555556000 ─────────────────────────────────────
                 [只读常量映射段  R--]
                 .rodata
                   └─ "%s %d %d %d\n" 等字符串常量
                 .eh_frame_hdr
                 .eh_frame

0x555555557db8 ─────────────────────────────────────
                 [可写数据映射段  RW-]
                 .init_array
                 .fini_array
                 .dynamic
                 .got
                   └─ printf@got.plt 等地址槽位
                 .data
                   ├─ g_init
                   └─ msg
                 .bss
                   └─ g_uninit

0x555555558028 ─────────────────────────────────────
```

内存上大致为：

```text
低地址
│
├─ 主程序映像（你的 elf_example）
│   ├─ 只读元信息段
│   ├─ 代码段
│   ├─ 只读数据段
│   └─ 可写数据段
│
├─ 堆（heap，brk/sbrk 扩展区域）
│
├─ 各种 mmap 区
│
├─ 共享库映像
│   ├─ libc.so.6
│   ├─ ld-linux-x86-64.so.2
│   └─ 其他 DSOs
│
├─ vDSO / vvar 等内核辅助映射
│
└─ 栈（stack）
高地址
```

## COMMON BLOCK

COMMON 是目标文件阶段对未初始化全局变量的一种临时归类方式，表示该变量尚未分配到具体 section，链接时再统一分配，最终通常进入 .bss

链接时，链接器会把所有 COMMON 符号收集起来，然后决定：

- 分配多大空间
- 按什么对齐
- 如果多个目标文件里有同名 COMMON，怎么合并
- 最终把它放到 .bss 或等价未初始化存储区

### nm

nm是Names缩写，作用是列出二进制文件中的符号表。

示例：

```c
/* a.c */
int x;          // 可能形成 COMMON
int y = 10;     // 一定在 .data
int z;          // 可能形成 COMMON

extern int w;   // 只是声明，不分配空间

int main() {
    return x + y + z + w;
}
/* b.c */
int x;          // 和 a.c 里的 x 同名
int z;          // 和 a.c 里的 z 同名
int w;          // 给 a.c 里的 extern int w 提供定义
```

使用nm工具

```bash
$ gcc -fcommon -c a.c
$ gcc -fcommon -c b.c
$ nm a.o
0000000000000000 T main
                 U w
0000000000000004 C x
0000000000000000 D y
0000000000000004 C z
$ nm b.o
0000000000000004 C w
0000000000000004 C x
0000000000000004 C z
```

C在这里面就表示COMMON，也就是弱符号。在`-fcommon`下，编译器先不把它们真正放到 .bss，而是记成 COMMON。

```bash
$ gcc -fcommon -o ab a.o b.o
$ nm ab | grep ' x\| y\| z\| w'
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 w __cxa_finalize@GLIBC_2.2.5
                 w __gmon_start__
0000000000004020 B w
000000000000401c B x
0000000000004010 D y
0000000000004018 B z
```

如果一开始就都是强定义放进 .bss，那链接可能会报冲突，在 -fcommon 机制下，链接器会把两个 COMMON 的 x 合并成一个最终实体。。

```bash
$ gcc -o ab a.o b.o
/usr/bin/ld: b.o:(.bss+0x0): multiple definition of `x'; a.o:(.bss+0x0): first defined here
/usr/bin/ld: b.o:(.bss+0x4): multiple definition of `z'; a.o:(.bss+0x4): first defined here
collect2: error: ld returned 1 exit status
```

## 链接脚本

手动控制内存布局的。平时写普通应用不需要它（因为 GCC 有默认规则），但当你需要精确控制内存（如写操作系统、驱动、嵌入式程序）或者想搞点黑客技术（如书中演示的极简程序）就可以用`.lds`文件。

## BFD库

BFD 是 Binutils 的核心引擎。它通过“屏蔽差异”，让 Linux 下的各种分析工具能够横跨各种平台。

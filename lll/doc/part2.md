# 编译与链接

## 隐藏过程

对于c程序：

1. 预处理：头文件+源文件被预处理后得到预处理文件`.i`
2. 编译：把预处理`.i`文件转成`.s`文件
3. 汇编：把`.s`文件转成`.o`文件
4. 链接：把各州`.o`，`.a`，`.so`文件链接成一个可执行文件

### 预处理

对于c，把源文件和`.h`头文件转为`.i`文件。对于c++，源文件`.cpp`或者`.cxx`，头文件扩展名可能为`.hpp`预处理后为`.ii`，第一步预处理相当于

```bash

gcc -E hello.c -o hello.i
# 或者
cpp hello.c > hello.i
```

预处理就是把源文件中以`#`开始的预编译指令展开：

1. 把所有`#define`去掉，并展开所有宏定义
2. 处理所有预编译指令，如`#if #ifdef #ifndef #endif #elif #else`
3. 处理`#include`预编译指令，将被包含的所有文件插入到该预编译指令的位置。（递归过程）
4. 删除所有的注释
5. 添加行号和文件标识，如:

    ```c
    # 0 "hello.c"
    # 0 "<built-in>"
    # 0 "<command-line>"
    # 1 "/usr/include/stdc-predef.h" 1 3 4
    # 0 "<command-line>" 2
    # 1 "hello.c"
    # 1 "./my_print.h" 1



    void my_print(char* str);
    # 2 "hello.c" 2

    int main()
    {

        char *str = "Hello World\n";

        print_str(str);

        return 0;
    }
    ```

    以便编译时编译器产生调试用的行号信息以及用于编译时产生的错误或警告时能显示行号。
    - 格式`# <行号> "<文件名>" <标志>`
    - `# 1 "hello.c"`告诉编译器从这里开始，是hello.c的第1行
    - `# 1 "./my_print.h 1"`：
        - 第一个`1`：表示当前文件的起始行号
        - `"./my_print.h"`：文件名
        - 末尾的`1`：标志位，表示进入了一个新的包含文件
    - `# 2 "hello.c" 2`：
        - 末尾的 2：表示从刚才的头文件返回
        - 这行告诉编译器：刚才处理完头文件了，现在回到了 hello.c 的第 2 行继续工作。
6. 保留所有的`#pragma`编译器符号，因为编译器要用。

若无法判断宏定义是否正确或头文件包含是否正确可以直接看`.i`文件

### 编译

把预处理文件进行一系列词法、语法、语义分析及优化后产生相应的汇编代码文件。通过：

```bash
gcc -S hello.i -o hello.s
```

得到如：

```bash
	.file	"hello.c"
	.text
	.section	.rodata
.LC0:
	.string	"Hello World\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	leaq	.LC0(%rip), %rax
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	print_str@PLT
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
```

中间下面部分很多人看，但上面没多少关注，对于

```bash
	.file	"hello.c"
	.text
	.section	.rodata
.LC0:
	.string	"Hello World\n"
	.text
	.globl	main
	.type	main, @function
```

首先`.file`声明源文件名，`.section.rodata`表示只读数据段，其中`.LC0`表示一个局部标签（Local Constant 0）代表后面字符串在内存中的首地址。`.type main @function`表示main是一个函数，对于Symbol Table很有用。

### 汇编

把汇编代码转变成机器可执行的指令。

```bash
as hello.s -o hello.o
# 或者
gcc -c hello.s -o hello.o
```

### 链接

```bash
gcc my_print.o hello.o -O0 -o hello
```

## 编译器工作

编译过程分6步：扫描、语法分析、语义分析、源代码优化、代码生成和目标代码优化。

### 词法分析

对于源程序

```c
a[i] = (i + 4) * (2 + 6); 
```

首先把源代码输入Scanner进行词法分析，产生的Token种类有：

关键字、标识符、字面量（包含数字、字符串等）和特殊符号（如加号、等号）

识别Token同时扫描器完成了其他工作，比如

- 把标识符存放到符号表
- 数字、字符串放到文字表

### 中间语言

可以用Clang查看中间语语言，用LLVM IR中间语言

```bash
clang -S -emit-llvm hello.c -o hello.ll
```

IR：平台无关，保留类型，方便优化器进行数字计算和逻辑简化。

## 链接器

### 目标文件格式

目标文件格式和可执行文件格式差不多，在Windows下是PE-COFF，在Linux下是ELF。

可在Linux下用File命令查看

```bash
$ file hello.o
hello.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
```

## ELF 文件

示例

```bash
$ objdump -h hello.o

hello.o:     file format elf64-x86-64

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  0 .text         0000002a  0000000000000000  0000000000000000  00000040  2**0
                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE
  1 .data         00000000  0000000000000000  0000000000000000  0000006a  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  2 .bss          00000000  0000000000000000  0000000000000000  0000006a  2**0
                  ALLOC
  3 .rodata       0000000d  0000000000000000  0000000000000000  0000006a  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .comment      0000002e  0000000000000000  0000000000000000  00000077  2**0
                  CONTENTS, READONLY
  5 .note.GNU-stack 00000000  0000000000000000  0000000000000000  000000a5  2**0
                  CONTENTS, READONLY
  6 .note.gnu.property 00000020  0000000000000000  0000000000000000  000000a8  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  7 .eh_frame     00000038  0000000000000000  0000000000000000  000000c8  2**3
                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, DATA
```

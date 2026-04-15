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

```text
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

```text
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

解析：

### 基础信息

`file format elf64-x86-64`一个64bit的ELF文件用于x86-64架构

`Size`大小、`VMA`虚存地址、`LMA`装载内存地址（在.o文件中全为0，因为还不确定）、`File off`该节在磁盘文件中的起始偏移位置。`Algn`对齐要求，就是2的多少次方。

### 核心段

`.text`代码段，属性`CONTENTS ALLOC LOAD RELOC READONLY CODE`

- RELOC表明需要重定位（.o文件是这样的）
- CONTENTS该节在文件中实际占有空间
- ALLOC告诉操作系统运行时，在内存给分配空间
- LOAD表示该部分需要从磁盘加载到内存
- CODE表示存的是机器指令

在内存中结构从低到高大致为

```text
+---------------------+
| ELF Header          |  文件总说明
+---------------------+
| Program Header Table|  给操作系统加载用
+---------------------+
| Section 1           |
| Section 2           |  真正的数据内容
| Section 3           |
| ...                 |
+---------------------+
| Section Header Table|  给链接器/分析工具看
+---------------------+
```

由于.bss的属性仅有ALLOC，实际上在ELF文件中是不占空间的。

### 段表

```bash
$ readelf -S hello.o
There are 14 section headers, starting at offset 0x268:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000000000  00000040
       000000000000002a  0000000000000000  AX       0     0     1
  [ 2] .rela.text        RELA             0000000000000000  000001a8
       0000000000000030  0000000000000018   I      11     1     8
  [ 3] .data             PROGBITS         0000000000000000  0000006a
       0000000000000000  0000000000000000  WA       0     0     1
  [ 4] .bss              NOBITS           0000000000000000  0000006a
       0000000000000000  0000000000000000  WA       0     0     1
  [ 5] .rodata           PROGBITS         0000000000000000  0000006a
       000000000000000d  0000000000000000   A       0     0     1
  [ 6] .comment          PROGBITS         0000000000000000  00000077
       000000000000002e  0000000000000001  MS       0     0     1
  [ 7] .note.GNU-stack   PROGBITS         0000000000000000  000000a5
       0000000000000000  0000000000000000           0     0     1
  [ 8] .note.gnu.pr[...] NOTE             0000000000000000  000000a8
       0000000000000020  0000000000000000   A       0     0     8
  [ 9] .eh_frame         PROGBITS         0000000000000000  000000c8
       0000000000000038  0000000000000000   A       0     0     8
  [10] .rela.eh_frame    RELA             0000000000000000  000001d8
       0000000000000018  0000000000000018   I      11     9     8
  [11] .symtab           SYMTAB           0000000000000000  00000100
       0000000000000090  0000000000000018          12     4     8
  [12] .strtab           STRTAB           0000000000000000  00000190
       0000000000000018  0000000000000000           0     0     1
  [13] .shstrtab         STRTAB           0000000000000000  000001f0
       0000000000000074  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
```

其中特殊的是`.rela.text`段，该段标注了代码中哪些地方需要链接器重定位。在这里Link标志位11（号段），链接器要在那里找函数名（.symtab），Info为1，也就是1号段`.text`，意味着`.rela.text`里所有的重定位指令都是为了修补`.text`中的机器码。

- 再比如`.symtab`段符号表，记录的是程序中所的函数和变量名。
  - link == 12，为`.strtab`。原理是`.symtab`内部不直接存储main, print_str这些字符，而是存一个数字偏移量。

Info表示最后一个局部符号的索引+1

- 在这里是4，意味着前3个符号是局部变量或局部标识，从第4个符号开始才是全局不好

`.symtab`符号表，记录了程序里定义的所有函数名和引用的外部名字
`.strtab`字符串表，符号表中的名字比如"main"其实是一串字符，记录这个
`.shstrtab`节头部字符串表，专门存这些节的名字（如.text, .data）本身的字符串

---
对应在`/usr/include/elf.h`中有

```c
typedef struct
{                                               
       Elf64_Word    sh_name;        /* Section name (string tbl inde     x) */
       Elf64_Word    sh_type;        /* Section type */
       Elf64_Xword   sh_flags;       /* Section flags */
       Elf64_Addr    sh_addr;        /* Section virtual addr at execution */
       Elf64_Off sh_offset;      /* Section file offset */
       Elf64_Xword   sh_size;        /* Section size in bytes */
       Elf64_Word    sh_link;        /* Link to another section */
       Elf64_Word    sh_info;        /* Additional section information */
       Elf64_Xword   sh_addralign;       /* Section alignment */
       Elf64_Xword   sh_entsize;     /* Entry size if section holds table */
} Elf64_Shdr;
```

readelf输出的结果就是ELF文件段表的内容。段表可理解为以`Elf64_Shdr`结构体为元素的数组，数组元素个数为段的个数，每个`Elf64_Shdr`结构体对应一个段。`Elf64_Shdr`也叫段描述符。

### ELF总体梳理

#### readelf工具

readelf查看elf文件结构

-h 查看elf头
-l 查看program header table
-S 查看Section header table
-s 查看符号表symtab
-r 查看重定位表
-d 查看动态段

#### 分类

ELF文件大致分三类：

1. 可重定位文件`.o`
2. 可执行文件`a.out main`
3. 共享目标文件`libc.so libm.so`

#### 总视角

##### 文件视角

有一堆section，这些由section header table统一描述

##### 装载视角

section被打包成segment，由program header table统一描述

##### 链接视角

symtab + strtab + reltab统一完成

#### 整体结构（大致）

```text
+--------------------------------------------------+
| ELF Header                                       |
|  - 文件类型、架构、入口地址                      |
|  - Program Header Table 偏移                     |
|  - Section Header Table 偏移                     |
+--------------------------------------------------+
| Program Header Table                             |
|  - 描述各个 Segment                              |
|  - 告诉操作系统：哪些内容要装入内存              |
|  - 每段装到哪、权限是什么                        |
+--------------------------------------------------+
| 各种 Section（逻辑分类内容）                     |
|  例如：                                          |
|  .interp        动态加载器路径                   |
|  .note.*        附加说明信息                     |
|  .dynsym        动态符号表                       |
|  .dynstr        动态字符串表                     |
|  .gnu.hash      动态符号查找辅助结构             |
|  .rela.dyn      动态重定位表                     |
|  .rela.plt      PLT 相关重定位表                 |
|  .init/.fini    初始化/结束代码                  |
|  .plt           外部函数调用跳板                 |
|  .text          程序代码                         |
|  .rodata        只读常量                         |
|  .dynamic       动态链接信息                     |
|  .got/.got.plt  地址表                           |
|  .data          已初始化全局/静态变量            |
|  .bss           未初始化全局/静态变量            |
|  .symtab        完整符号表（分析/调试）          |
|  .strtab        完整字符串表                     |
|  .shstrtab      section 名字字符串表             |
+--------------------------------------------------+
| Section Header Table                             |
|  - 描述每个 Section                              |
|  - 名字、类型、偏移、大小、属性                  |
+--------------------------------------------------+
```

执行ELF文件，内核先读ELF Header Table，检查完后内核按照Program Header Table装载Segment，然后动态链接器处理动态链接，然后从入口地址开始执行

#### 关于段Segment和节Section区别

Section是给编译器/链接器/程序员看的：

- bss：未初始化全局变量
- data：已初始化全局变量
- rodata：只读数据
- symtab：符号表
- rel.text：代码段相关的重定位信息
- text：代码

Section更偏向逻辑分类

Segment是给OS看的，在装载运行很关键。OS更关心

- 该部分要不要装载到内存
- 装载到哪里
- 读写权限是什么
- 是否可执行

多个Section合并为一个Segment，如.data+.bss可能放到一个可读可写的段。

#### ELF HEADER

ELF开头就是HEADER，是整个文件的总目录。有什么魔数、程序入口、Program Header Table的偏移、Section Header Table的偏移、当前文件类型（REL, EXEC, DYN）

#### Program Header Table

SHT按内容分类
PHT按加载需求打包

问答的问题是：“程序运行时，应该把文件哪几块映射到内存”，“映射到哪个VMA”，“权限是什么”，“哪部分需要动态链接器处理”。

通过`readelf -l elf_example`可看

```text
Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000000040 0x0000000000000040
                 0x00000000000002d8 0x00000000000002d8  R      0x8
  INTERP         0x0000000000000318 0x0000000000000318 0x0000000000000318
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000640 0x0000000000000640  R      0x1000
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000
                 0x00000000000001bd 0x00000000000001bd  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000002000 0x0000000000002000
                 0x0000000000000124 0x0000000000000124  R      0x1000
  LOAD           0x0000000000002db8 0x0000000000003db8 0x0000000000003db8
                 0x0000000000000268 0x0000000000000270  RW     0x1000
  DYNAMIC        0x0000000000002dc8 0x0000000000003dc8 0x0000000000003dc8
                 0x00000000000001f0 0x00000000000001f0  RW     0x8
  NOTE           0x0000000000000338 0x0000000000000338 0x0000000000000338
                 0x0000000000000030 0x0000000000000030  R      0x8
  NOTE           0x0000000000000368 0x0000000000000368 0x0000000000000368
                 0x0000000000000044 0x0000000000000044  R      0x4
  GNU_PROPERTY   0x0000000000000338 0x0000000000000338 0x0000000000000338
                 0x0000000000000030 0x0000000000000030  R      0x8
  GNU_EH_FRAME   0x0000000000002018 0x0000000000002018 0x0000000000002018
                 0x000000000000003c 0x000000000000003c  R      0x4
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000002db8 0x0000000000003db8 0x0000000000003db8
                 0x0000000000000248 0x0000000000000248  R      0x1
```

关键内容：

`LOAD`： 这是一个要装进内存的段

上述示例有四个`LOAD`段，最开始是文件头和动态链接元数据，从0x0开始到0x640，只读，这里面装的一般不是业务代码或者普通变量，一般是各种Note信息，相当于程序的说明书，动态链接信息，重定位信息所在的只读区域。

第二个LOAD是RE经典代码段，通常有`.init .plt .plt.got .text`等等，也就是各种main add函数在这里，print@plt也在这里

第三个LOAD是R只读数据段，里面就是“只读常量区 + 异常展开/调试辅助信息”。

第四个LOAD是RW可写数据段，程序运行时可修改的数据区，这里经常MemSiz > FileSiz，因为.bss通常在尾部，最开始是不占用文件空间，但加载到内存是要分配的。

`INTERP`

告诉内核这个程序需要哪个动态链接器来解释执行，示例中`[Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]`就是Linux下常见的动态加载器。

`DYNAMIC`

告诉动态链接器，动态库依赖信息在哪，REL表在哪，GOT/POT相关信息在哪

`Section to Segment mapping`

表示Section和Segment的关联。比如03行`03     .init .plt .plt.got .plt.sec .text .fini`就是编号为03的段里面装的是这些section。也就说明多个Section会被打包进入一个Segment里，OS按照Segment装载。

#### Section：ELF中真正逻辑块

1. .text代码段，程序机器指令（只读可执行）
2. .data已经初始化的全局变量和静态变量（可读可写，文件中真的占空间）
3. .bss未初始化的全局变量和静态变量（或者初值为0的）（在文件中一般不存内容，但需要后续分配到内存运行时给一定空间只是文件中没占空间）
4. .rodata只读数据段（可读不可写）
5. .symtab符号表，记录函数变量名等信息，记录名称和地址的对应关系，记录有：符号名、地址、大小、所属Section、是局部还是全局、是函数还是对象等等
6. .strtab字符串表（.symtab记录的是偏移值）这里.strtab记录的是真正的名字.
    - .symtab某项记录了name=25，那表示符号名从.strtab第25个字节开始存了一个字符如"main"
7. .shstrtab节名字字符串表，专门给section用的。记录的是这个节名字从 .shstrtab 的某个偏移开始，然后找到对应的Section
8. .rel.text./.rela.text重定位表
9. .plt（Procedure Linkage Table）过程链接表，与动态链接相关。
    - 比如程序调用库函数`printf("hi\n");`，printf真正地址在程序运行前不一定已经确定，所以程序会通过`.plt`做一次跳板跳转，相当于.plt是调用动态链接库函数时的中转站。
10. .got（Global Offset Table）全局偏移表，动态链接相关，存放一些在运行时确定的地址，比如某个动态链接库函数的真实地址，第一次调用时还没写进去，等动态链接器解析完，在填到GOT中，后续调用直接从GOT中取地址
11. .dynamic动态链接相关信息表，告诉动态链接器，需要哪些共享库、动态符号哪里、重定位表REL在哪里，GOT PLT在哪里

#### Section Header Table

前面说是具体的Section，但现在要让OS知道到底在哪里，大小多少，啥类型，就要靠SHT，相当于`节目录表`

每一项对应一个section，描述

- 节名称
- 节类型
- 节权限属性
- 在文件中的偏移
- 大小
- 对齐方式
- 关联信息

`readelf -S elf_example`查看符号表

```text
Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .interp           PROGBITS         0000000000000318  00000318
       000000000000001c  0000000000000000   A       0     0     1
  [ 2] .note.gnu.pr[...] NOTE             0000000000000338  00000338
       0000000000000030  0000000000000000   A       0     0     8
  [ 3] .note.gnu.bu[...] NOTE             0000000000000368  00000368
       0000000000000024  0000000000000000   A       0     0     4
  [ 4] .note.ABI-tag     NOTE             000000000000038c  0000038c
       0000000000000020  0000000000000000   A       0     0     4
  [ 5] .gnu.hash         GNU_HASH         00000000000003b0  000003b0
       0000000000000024  0000000000000000   A       6     0     8
  [ 6] .dynsym           DYNSYM           00000000000003d8  000003d8
       00000000000000a8  0000000000000018   A       7     1     8
  [ 7] .dynstr           STRTAB           0000000000000480  00000480
       000000000000008f  0000000000000000   A       0     0     1
  [ 8] .gnu.version      VERSYM           0000000000000510  00000510
       000000000000000e  0000000000000002   A       6     0     2
  [ 9] .gnu.version_r    VERNEED          0000000000000520  00000520
       0000000000000030  0000000000000000   A       7     1     8
  [10] .rela.dyn         RELA             0000000000000550  00000550
       00000000000000d8  0000000000000018   A       6     0     8
  [11] .rela.plt         RELA             0000000000000628  00000628
       0000000000000018  0000000000000018  AI       6    24     8
  [12] .init             PROGBITS         0000000000001000  00001000
       000000000000001b  0000000000000000  AX       0     0     4
  [13] .plt              PROGBITS         0000000000001020  00001020
       0000000000000020  0000000000000010  AX       0     0     16
  [14] .plt.got          PROGBITS         0000000000001040  00001040
       0000000000000010  0000000000000010  AX       0     0     16
  [15] .plt.sec          PROGBITS         0000000000001050  00001050
       0000000000000010  0000000000000010  AX       0     0     16
  [16] .text             PROGBITS         0000000000001060  00001060
       000000000000014e  0000000000000000  AX       0     0     16
  [17] .fini             PROGBITS         00000000000011b0  000011b0
       000000000000000d  0000000000000000  AX       0     0     4
  [18] .rodata           PROGBITS         0000000000002000  00002000
       0000000000000017  0000000000000000   A       0     0     4
  [19] .eh_frame_hdr     PROGBITS         0000000000002018  00002018
       000000000000003c  0000000000000000   A       0     0     4
  [20] .eh_frame         PROGBITS         0000000000002058  00002058
       00000000000000cc  0000000000000000   A       0     0     8
  [21] .init_array       INIT_ARRAY       0000000000003db8  00002db8
       0000000000000008  0000000000000008  WA       0     0     8
  [22] .fini_array       FINI_ARRAY       0000000000003dc0  00002dc0
       0000000000000008  0000000000000008  WA       0     0     8
  [23] .dynamic          DYNAMIC          0000000000003dc8  00002dc8
       00000000000001f0  0000000000000010  WA       7     0     8
  [24] .got              PROGBITS         0000000000003fb8  00002fb8
       0000000000000048  0000000000000008  WA       0     0     8
  [25] .data             PROGBITS         0000000000004000  00003000
       0000000000000020  0000000000000000  WA       0     0     8
  [26] .bss              NOBITS           0000000000004020  00003020
       0000000000000008  0000000000000000  WA       0     0     4
  [27] .comment          PROGBITS         0000000000000000  00003020
       000000000000002d  0000000000000001  MS       0     0     1
  [28] .symtab           SYMTAB           0000000000000000  00003050
       00000000000003c0  0000000000000018          29    18     8
  [29] .strtab           STRTAB           0000000000000000  00003410
       00000000000001f5  0000000000000000           0     0     1
  [30] .shstrtab         STRTAB           0000000000000000  00003605
       000000000000011a  0000000000000000
```

比如

```text
[16] .text             PROGBITS         0000000000001060  00001060
       000000000000014e  0000000000000000  AX       0     0     16
```

- [16]这是第16个section
- .text节名
- PROGBITS节类型
- Flags=AX，A=alloc运行时要装入内存分配空间；X可执行

针对sh_type，有

- PROGBITS普通内容节，真正存放字节，比如`.text .rodata. .data .plt .got`常常是这个类型，意思就是这里真的有数据
- NOBITS表示逻辑上有这块节，但文件里不存这些字节，典型就是`.bss`
- SYMTAB就是符号表
- STRTAB字符串表比如`.strtab .shstrtab. dynstr`
- RELA带addend（附加值）重定位表，最终值=符号地址+addend+其他修正
- DYNAMIC动态链接信息表

#### 符号表.symtab

通过`readelf -s elf_example`查看符号表

输出有两张表`.dynsym .symtab`

示例：

```text
Symbol table '.dynsym' contains 7 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND _[...]@GLIBC_2.34 (2)
     2: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_deregisterT[...]
     3: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND [...]@GLIBC_2.2.5 (3)
     4: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__
     5: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_registerTMC[...]
     6: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND [...]@GLIBC_2.2.5 (3)

Symbol table '.symtab' contains 40 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS Scrt1.o
     2: 000000000000038c    32 OBJECT  LOCAL  DEFAULT    4 __abi_tag
     3: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crtstuff.c
     4: 0000000000001090     0 FUNC    LOCAL  DEFAULT   16 deregister_tm_clones
     5: 00000000000010c0     0 FUNC    LOCAL  DEFAULT   16 register_tm_clones
     6: 0000000000001100     0 FUNC    LOCAL  DEFAULT   16 __do_global_dtors_aux
     7: 0000000000004020     1 OBJECT  LOCAL  DEFAULT   26 completed.0
     8: 0000000000003dc0     0 OBJECT  LOCAL  DEFAULT   22 __do_global_dtor[...]
     9: 0000000000001140     0 FUNC    LOCAL  DEFAULT   16 frame_dummy
    10: 0000000000003db8     0 OBJECT  LOCAL  DEFAULT   21 __frame_dummy_in[...]
    11: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS efl_example.c
    12: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS crtstuff.c
    13: 0000000000002120     0 OBJECT  LOCAL  DEFAULT   20 __FRAME_END__
    14: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS
    15: 0000000000003dc8     0 OBJECT  LOCAL  DEFAULT   23 _DYNAMIC
    16: 0000000000002018     0 NOTYPE  LOCAL  DEFAULT   19 __GNU_EH_FRAME_HDR
    17: 0000000000003fb8     0 OBJECT  LOCAL  DEFAULT   24 _GLOBAL_OFFSET_TABLE_
    18: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __libc_start_mai[...]
    19: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_deregisterT[...]
    20: 0000000000004000     0 NOTYPE  WEAK   DEFAULT   25 data_start
    21: 0000000000001149    24 FUNC    GLOBAL DEFAULT   16 add
    22: 0000000000004024     4 OBJECT  GLOBAL DEFAULT   26 g_uninit
    23: 0000000000004020     0 NOTYPE  GLOBAL DEFAULT   25 _edata
    24: 00000000000011b0     0 FUNC    GLOBAL HIDDEN    17 _fini
    25: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND printf@GLIBC_2.2.5
    26: 0000000000004000     0 NOTYPE  GLOBAL DEFAULT   25 __data_start
    27: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__
    28: 0000000000004008     0 OBJECT  GLOBAL HIDDEN    25 __dso_handle
    29: 0000000000002000     4 OBJECT  GLOBAL DEFAULT   18 _IO_stdin_used
    30: 0000000000004028     0 NOTYPE  GLOBAL DEFAULT   26 _end
    31: 0000000000001060    38 FUNC    GLOBAL DEFAULT   16 _start
    32: 0000000000004020     0 NOTYPE  GLOBAL DEFAULT   26 __bss_start
    33: 0000000000001161    77 FUNC    GLOBAL DEFAULT   16 main
    34: 0000000000004010     4 OBJECT  GLOBAL DEFAULT   25 g_init
    35: 0000000000004020     0 OBJECT  GLOBAL HIDDEN    25 __TMC_END__
    36: 0000000000000000     0 NOTYPE  WEAK   DEFAULT  UND _ITM_registerTMC[...]
    37: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND __cxa_finalize@G[...]
    38: 0000000000001000     0 FUNC    GLOBAL HIDDEN    12 _init
    39: 0000000000004018     8 OBJECT  GLOBAL DEFAULT   25 msg
```

比如：

```text
34: 0000000000004010     4 OBJECT  GLOBAL DEFAULT   25 g_init
```

编号34，地址0x4010，大小4B，类型OBJECT也就是对象（变量），属性GLOBAL全局变量，所在section号25（` [25] .data PROGBITS ... WA`）名称是g_init

特殊一行：

```text
25: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND printf@GLIBC_2.2.5
```

这里section是UND，这说明当前文件没定义，需要在动态库中找

#### 字符串表.strtab

使用`readelf -p .strtab elf_example`可以看当前文件的字符串表

```text
String dump of section '.strtab':
  [     1]  Scrt1.o
  [     9]  __abi_tag
  [    13]  crtstuff.c
  [    1e]  deregister_tm_clones
  [    33]  __do_global_dtors_aux
  [    49]  completed.0
  [    55]  __do_global_dtors_aux_fini_array_entry
  [    7c]  frame_dummy
  [    88]  __frame_dummy_init_array_entry
  [    a7]  efl_example.c
  [    b5]  __FRAME_END__
  [    c3]  _DYNAMIC
  [    cc]  __GNU_EH_FRAME_HDR
  [    df]  _GLOBAL_OFFSET_TABLE_
  [    f5]  __libc_start_main@GLIBC_2.34
  [   112]  _ITM_deregisterTMCloneTable
  [   12e]  add
  [   132]  g_uninit
  [   13b]  _edata
  [   142]  _fini
  [   148]  printf@GLIBC_2.2.5
  [   15b]  __data_start
  [   168]  __gmon_start__
  [   177]  __dso_handle
  [   184]  _IO_stdin_used
  [   193]  _end
  [   198]  __bss_start
  [   1a4]  main
  [   1a9]  g_init
  [   1b0]  __TMC_END__
  [   1bc]  _ITM_registerTMCloneTable
  [   1d6]  __cxa_finalize@GLIBC_2.2.5
  [   1f1]  msg
```

字符串表本质上是：一大串以 \0 结尾的字符串拼在一起。前面`[13] [1e]`就是偏移量，符号表里面存的是偏移量，字符串表存的是字符串名字。比如`[   1a4]  main`名字是从.strtab偏移处0x1a4开始，然后去.strtab找到main

关系图形如：

```text
.symtab:
[entry for main] ----st_name=0x1a1----> .strtab 中的 "main"
[entry for g_init] ---st_name=0x1a6----> .strtab 中的 "g_init"

Section Header:
[entry for .text] ----sh_name=0xa2----> .shstrtab 中的 ".text"
[entry for .data] ----sh_name=0x106---> .shstrtab 中的 ".data"
```

故：

- 符号表不直接存名字，而是存名字偏移
- 节头表也不直接存名字，而是存节名偏移

#### .plt与.got

`Global Offset Table`

本质是地址表，放的是动态符号最终解析出的真实地址和运行需要的全局地址

`Procedure Linkage Table`

一组跳板代码。程序调用动态库的函数时，不是直接跳真实地址，而是先跳到.plt的某个入口，偏“代码”。

.plt复杂跳，.got负责存地址。`call   1050 <printf@plt>`可以看到主程不是直接去libc里面找printf而是先跳到plt中。

在objdump反汇编后有：

```bash
0000000000001050 <printf@plt>:
    1050:       f3 0f 1e fa             endbr64
    1054:       ff 25 76 2f 00 00       jmp    *0x2f76(%rip)        # 3fd0 <printf@GLIBC_2.2.5>
    105a:       66 0f 1f 44 00 00       nopw   0x0(%rax,%rax,1)
```

1.`jmp *0x2f76(%rip) # 3fd0 <printf@GLIBC_2.2.5>`去.got.plt去printf的地址，然后跳过去
    - 如果是真实地址就跳到libc中的
    - 如果不是，会push 索引, jmp plt0进入动态链接器，让其解析地址，并填回GOT

```bash
1020:       ff 35 9a 2f 00 00       push   0x2f9a(%rip)
```

这是PLT0是总入口。

`.got.plt`是给.plt配套用的地址表，负责记录真实地址。

`.plt.got`通过 GOT 间接跳转的辅助跳板区

第一次调用printf（懒绑定）

```text
main
 -> 进入 printf 对应的 .plt 入口
 -> .plt 发现 .got.plt 里还没有 printf 的真实地址
 -> 去找动态链接器
 -> 动态链接器找到 libc 里的 printf
 -> 把 printf 真实地址写进 .got.plt
 -> 跳到真正的 printf
```

第二次调用printf

```text
main
 -> 进入 printf 对应的 .plt 入口
 -> .plt 查 .got.plt
 -> 发现里面已经有真实地址
 -> 直接跳到真正的 printf
```

#### GDB调试过程

```bash
pwndbg> info files
Symbols from "/home/jason/ck_study/lll/code/part2/elf_example".
Local exec file:
        `/home/jason/ck_study/lll/code/part2/elf_example', file type elf64-x86-64.
        Entry point: 0x1060
        0x0000000000000318 - 0x0000000000000334 is .interp
        0x0000000000000338 - 0x0000000000000368 is .note.gnu.property
        0x0000000000000368 - 0x000000000000038c is .note.gnu.build-id
        0x000000000000038c - 0x00000000000003ac is .note.ABI-tag
        0x00000000000003b0 - 0x00000000000003d4 is .gnu.hash
        0x00000000000003d8 - 0x0000000000000480 is .dynsym
        0x0000000000000480 - 0x000000000000050f is .dynstr
        0x0000000000000510 - 0x000000000000051e is .gnu.version
        0x0000000000000520 - 0x0000000000000550 is .gnu.version_r
        0x0000000000000550 - 0x0000000000000628 is .rela.dyn
        0x0000000000000628 - 0x0000000000000640 is .rela.plt
        0x0000000000001000 - 0x000000000000101b is .init
        0x0000000000001020 - 0x0000000000001040 is .plt
        0x0000000000001040 - 0x0000000000001050 is .plt.got
        0x0000000000001050 - 0x0000000000001060 is .plt.sec
        0x0000000000001060 - 0x00000000000011ae is .text
        0x00000000000011b0 - 0x00000000000011bd is .fini
        0x0000000000002000 - 0x0000000000002017 is .rodata
        0x0000000000002018 - 0x0000000000002054 is .eh_frame_hdr
        0x0000000000002058 - 0x0000000000002124 is .eh_frame
        0x0000000000003db8 - 0x0000000000003dc0 is .init_array
        0x0000000000003dc0 - 0x0000000000003dc8 is .fini_array
        0x0000000000003dc8 - 0x0000000000003fb8 is .dynamic
        0x0000000000003fb8 - 0x0000000000004000 is .got
        0x0000000000004000 - 0x0000000000004020 is .data
        0x0000000000004020 - 0x0000000000004028 is .bss
```

其中`Entry point: 0x1060`这就是e_entry（程序的入口点地址），通过readelf也能证实这一点`Entry point address:0x1060`。

然后通过`starti`看第一条机器指令，也就是看_start，通过`x/i $pc`看当前第一条机器指令，和bt看调用栈

```bash
0x7ffff7fe4540 <_start>               mov    rdi, rsp     RDI => 0x7fffffffdcf0 ◂— 1
---
#0  0x00007ffff7fe4540 in _start () from /lib64/ld-linux-x86-64.so.2
```

首先能看出当前程序是一个动态链接程序（是ld-linux的_start），程序最开始执行的不是程序文件中的代码，而是`ld-linux-x86-64.so.2`也就是动态加载器，它负责做一些准备工作

- 把程序映射到内存
- 把依赖的动态库加载进来
- 处理部分动态链接事务
- 最后把控制权交给自己程序入口

现在再info files看一下情况

```bash
Entry point: 0x555555555060
...
0x0000555555555050 - 0x0000555555555060 is .plt.sec
0x0000555555555060 - 0x00005555555551ae is .text
```

表示程序自己的入口。注意：动态加载器的入口是0x7ffff7fe4540。这里可以看出0x555555555060正好就是.text的入口点。

对于自己的_start采用`x/10i 0x555555555060`看出10条指令为：

```bash
0x555555555060 <_start>:     endbr64
0x555555555064 <_start+4>:   xor    ebp,ebp
0x555555555066 <_start+6>:   mov    r9,rdx
0x555555555069 <_start+9>:   pop    rsi
0x55555555506a <_start+10>:  mov    rdx,rsp
0x55555555506d <_start+13>:  and    rsp,0xfffffffffffffff0
0x555555555071 <_start+17>:  push   rax
0x555555555072 <_start+18>:  push   rsp
0x555555555073 <_start+19>:  xor    r8d,r8d
0x555555555076 <_start+22>:  xor    ecx,ecx
```

这是自己的_start，它的作用不是直接执行业务代码，是把**运行环境整理好，然后调用libc启动流程，最后到main**

逻辑梳理为：

```text
ld-linux 的 _start
 -> 你程序自己的 _start
 -> libc 启动逻辑
 -> main
```

---

【补充】libc也就是Glibc(GNU C Library)是Linux中C标准库，是当前程序与linux内核的桥梁：封装系统调用，提供通用功能（malloc,sin,strlen,...），程序启动与退出

_start是一段汇编代码，通常包含在crt1.o启动文件中：

1. OS把命令行参数argc,argv和环境变量压栈，_start负责从栈中把这些数据取出来
2. 调用libc中核心函数`__libc_start_main`，告诉_start main函数在哪里，初始化一些函数（如c++中构造函数），析构函数，设置线程局部存储TLS，初始化标准输入输出，等等，然后会调用main函数。

3. 如果__lib_start_main返回了（正常情况不会返回而是接管流程），_start会调用系统调用exit结束程序

---

在_start的汇编程序中，可以看到有几行汇编代码：

```bash
0x0000555555555078 <+24>:    lea    rdi,[rip+0xe2]        # 0x555555555161 <main>
0x000055555555507f <+31>:    call   QWORD PTR [rip+0x2f53]        # 0x555555557fd8
```

第一行lea表明_start已经拿到了main函数地址
第二行call实际上调用的是地址表中的一个函数指针，这个地址在0x555555557fd8，而这个地址对应的是`0x0000555555557fb8 - 0x0000555555558000 is .got`是GOT，但当前肯定是全0，因为刚开始啥都没确定。

现在直接运行到main部分看看调用栈

```bash
 ► 0   0x555555555169 main+8
   1   0x7ffff7c2a1ca __libc_start_call_main+122
   2   0x7ffff7c2a28b __libc_start_main+139
   3   0x555555555085 _start+37
```

说明现在已经进入main函数，而main函数是被__libc_start_call_main函数启动的

对于main函数反汇编：

```bash
0x0000555555555173 <+18>:    call   0x555555555149 <add>
...
0x00005555555551a2 <+65>:    call   0x555555555050 <printf@plt>
```

第一行就是调用自己的add，就定义在当前elf中直接就用了。第二行可以看到是先到plt跳板中。

根据当前环境继续看调用过程

```bash
0x555555555050 <printf@plt>: endbr64
0x555555555054 <printf@plt+4>:       jmp    QWORD PTR [rip+0x2f76]        # 0x555555557fd0 <printf@got.plt>
0x55555555505a <printf@plt+10>:      nop    WORD PTR [rax+rax*1+0x0]
0x555555555060 <_start>:     endbr64
0x555555555064 <_start+4>:   xor    ebp,ebp
0x555555555066 <_start+6>:   mov    r9,rdx
```

直接看`jmp QWORD PTR [rip+0x2f76] # 0x555555557fd0 <printf@got.plt>`可以看出print@plt的核心工作就是跳转到printf@got.plt

继续`x/gx 0x555555557fd0`可看到`0x555555557fd0 <printf@got.plt>: 0x00007ffff7c60100`

意思是 printf@got.plt 这个槽位里，现在存着一个真实地址 0x7ffff7c60100。之前GOT里面全是0，但现在不是，这说明：GOT/GOT.PLT 里的槽位内容是运行时动态建立的，不同槽位、不同阶段，状态可能不同。

通过`info symbol 0x00007ffff7c60100`和`x/5i 0x00007ffff7c60100`有

```bash
printf in section .text of /lib/x86_64-linux-gnu/libc.so.6
---
0x7ffff7c60100 <__printf>:   endbr64
0x7ffff7c60104 <__printf+4>: push   rbp
0x7ffff7c60105 <__printf+5>: mov    rbp,rsp
0x7ffff7c60108 <__printf+8>: sub    rsp,0xd0
0x7ffff7c6010f <__printf+15>:        mov    QWORD PTR [rbp-0xa8],rsi
```

这两条合起来说明：printf@got.plt 里存的那个地址，确实就是 libc 里的 printf 实现入口。

当前情况是：

```text
main
 -> call printf@plt
 -> printf@plt 去读 printf@got.plt
 -> printf@got.plt = 0x7ffff7c60100
 -> 这个地址对应 libc.so.6 里的 printf / __printf
```

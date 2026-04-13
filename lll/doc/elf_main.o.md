```c
#include <stdio.h>

int count = 10;
int value;

void func(int sum)
{
    printf("sum is : %d\n", sum);
}

int main()
{
    static int a = 1;
    static int b = 0;
    int x = 1;
    func(a + b + x);
    return 0;
}
```



objdump:

```bash
main.o:     file format elf64-x86-64

Contents of section .text:
 0000 f30f1efa 554889e5 4883ec10 897dfc8b  ....UH..H....}..
 0010 45fc89c6 488d0500 00000048 89c7b800  E...H......H....
 0020 000000e8 00000000 90c9c3f3 0f1efa55  ...............U
 0030 4889e548 83ec10c7 45fc0100 00008b15  H..H....E.......
 0040 00000000 8b050000 000001c2 8b45fc01  .............E..
 0050 d089c7e8 00000000 b8000000 00c9c3    ...............
Contents of section .data:
 0000 0a000000 01000000                    ........
Contents of section .rodata:
 0000 73756d20 6973203a 2025640a 00        sum is : %d..
Contents of section .comment:
 0000 00474343 3a202855 62756e74 75203133  .GCC: (Ubuntu 13
 0010 2e332e30 2d367562 756e7475 327e3234  .3.0-6ubuntu2~24
 0020 2e303429 2031332e 332e3000           .04) 13.3.0.
Contents of section .note.gnu.property:
 0000 04000000 10000000 05000000 474e5500  ............GNU.
 0010 020000c0 04000000 03000000 00000000  ................
Contents of section .eh_frame:
 0000 14000000 00000000 017a5200 01781001  .........zR..x..
 0010 1b0c0708 90010000 1c000000 1c000000  ................
 0020 00000000 2b000000 00450e10 8602430d  ....+....E....C.
 0030 06620c07 08000000 1c000000 3c000000  .b..........<...
 0040 00000000 34000000 00450e10 8602430d  ....4....E....C.
 0050 066b0c07 08000000                    .k......

Disassembly of section .text:

0000000000000000 <func>:
   0:   f3 0f 1e fa             endbr64
   4:   55                      push   %rbp
   5:   48 89 e5                mov    %rsp,%rbp
   8:   48 83 ec 10             sub    $0x10,%rsp
   c:   89 7d fc                mov    %edi,-0x4(%rbp)
   f:   8b 45 fc                mov    -0x4(%rbp),%eax
  12:   89 c6                   mov    %eax,%esi
  14:   48 8d 05 00 00 00 00    lea    0x0(%rip),%rax        # 1b <func+0x1b>
  1b:   48 89 c7                mov    %rax,%rdi
  1e:   b8 00 00 00 00          mov    $0x0,%eax
  23:   e8 00 00 00 00          call   28 <func+0x28>
  28:   90                      nop
  29:   c9                      leave
  2a:   c3                      ret

000000000000002b <main>:
  2b:   f3 0f 1e fa             endbr64
  2f:   55                      push   %rbp
  30:   48 89 e5                mov    %rsp,%rbp
  33:   48 83 ec 10             sub    $0x10,%rsp
  37:   c7 45 fc 01 00 00 00    movl   $0x1,-0x4(%rbp)
  3e:   8b 15 00 00 00 00       mov    0x0(%rip),%edx        # 44 <main+0x19>
  44:   8b 05 00 00 00 00       mov    0x0(%rip),%eax        # 4a <main+0x1f>
  4a:   01 c2                   add    %eax,%edx
  4c:   8b 45 fc                mov    -0x4(%rbp),%eax
  4f:   01 d0                   add    %edx,%eax
  51:   89 c7                   mov    %eax,%edi
  53:   e8 00 00 00 00          call   58 <main+0x2d>
  58:   b8 00 00 00 00          mov    $0x0,%eax
  5d:   c9                      leave
  5e:   c3                      ret
```



## 分析：

首先看elf文件中header部分`readelf -h main.o`

```bash
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          1000 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           64 (bytes)
  Number of section headers:         14
  Section header string table index: 13
```

主要是Magic: 7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 （魔数：用于确认文件类型）

- 7f 'DEL' 45 'E' 4c 'L' 46'F' (ASCII)
- 02：ELF文件类型
  - 0x01 32bit
  - 0x02 64bit
- 01字节序
  - 0x01小端
  - 0x02大端
- 01ELF文件版本号，一般为1
- 00 00 ... 00 最后9B中无定义用0填充

### 有哪些可以进入ELF段

全局/静态 （有static storage duration） 会进入.data/.bss/.rodata

```c
int count = 10;   // 全局，初始化非0  → .data
int value;        // 全局，未初始化     → .bss

static int a = 1; // main 里的静态，初始化非0 → .data（但符号是局部/内部链接）
static int b = 0; // main 里的静态，初始化为0 → 通常放 .bss（也可能放 .bss）

"sum is : %d\n" // 进入.rodata
```

局部变量放在栈STACK上，不进入.bss/.data中

```c
int x = 1;        // 在 main 的栈帧里
int sum (func参数) // 通过寄存器传参（%edi）
```



```
.text        → 机器指令（核心）
.data        → 已初始化的全局变量/静态C变量
.rodata      → 只读数据（字符串常量）
.bss         → （这里没有直接 dump，但你已经理解了）
.rela.text   → （没显示，但一定存在，用于重定位）
.symtab      → 符号表（objdump 默认不显示）
.eh_frame    → 异常展开信息（可忽略）
.note / .comment → 元数据（可忽略）
```

- .data 这是真存在文件中的数据

  - ```
    Contents of section .data:
     0000 0a000000 01000000   
    ```

  - 按小段解释

  - ```
    0a 00 00 00  → 10
    01 00 00 00  → 1
    ```

    - 说明程序中有两个初始话的全局或静态变量是0和1

- rodata 只读数据（常量），比如printf中的字符串或switch语句的跳转表

  - ```
    Contents of section .rodata:
     0000 73756d20 6973203a 2025640a 00
    ```

  - ASCII码：

    ```bash
    73 75 6d 20 69 73 20 3a 20 25 64 0a 00
     s  u  m     i  s     :     %  d \n \0(表示字符串结尾)
    ```

- .text 已编译 程序的机器代码⭐

  - ```
    0000000000000000 <func>:
    ...
    000000000000002b <main>:
    ```

  - 说明源代中含有

    ```C
    void func(int);
    int main(void);
    ```

    上述两个函数

  - 解析func，函数入口

    - ```asm
      0:  f3 0f 1e fa      endbr64
      4:  55               push %rbp
      5:  48 89 e5         mov %rsp,%rbp
      8:  48 83 ec 10      sub $0x10,%rsp
      ```

    - **函数序言（prologue）**

      - `endbr64`：控制流保护（忽略）
      - 建立栈帧
      - 分配 16 字节栈空间

  - 参数保存`c:  89 7d fc         mov %edi,-0x4(%rbp)`

  - 含义：

    - `%edi`：第一个 int 参数
    - 存到栈上的局部变量(-O0常用手段便于gdb调试，都会放到栈上，但-O2可能就没有了)
    - 对应c `int x = 1;`

  - 准备调用printf

    - ```asm
      f:   8b 45 fc        mov -0x4(%rbp),%eax
      12:  89 c6           mov %eax,%esi
      ```

    - 第二个参数%esi = 要打印的整数

      - eax除了作为返回值，也可作为**中转寄存器**，只是-O0下就习惯有一个从内存到寄存器之间有一个中转，然后准备第二个参数sum,printf的第二个参数

    - ```asm
      14:  48 8d 05 00 00 00 00   lea 0(%rip), %rax
      1b:  48 89 c7               mov %rax,%rdi
      ```

    - 第一个参数（`%rdi`）= 字符串地址

      - 在 x86-64 中，访问**全局/静态对象**的地址，必须使用 RIP-relative（**相对当前指令地址**）方式
      - 就是你要用字符串常量，不允许直接写绝对地址，必须PC相对寻址的方式（偏移量）
      - rax也是中转，后面给rdi就有了Printf函数的第一个参数

      **这里是重定位占位符**（现在还是 0）

    - ```asm
      1e:  b8 00 00 00 00         mov $0,%eax
      23:  e8 00 00 00 00         call <printf>
      ```

      - `%al`这是寄存器`%rax`低8位

        - 可变参数函数调用前，`%al` 里要放浮点参数个数，`%al` 中保存通过 XMM 寄存器传递的浮点参数个数，只用8位就行不用全部

      - 调用外部函数（`printf`）

        `call` 的目标地址现在未知 → **重定位**

  - leave指令等价于

    - ```asm
      mov %rbp, %rsp
      pop %rbp
      ```

    - 销毁当前函数的栈帧，回到调用者的栈环境

## 符号表

查看符号表Symbol table.symtab

```bash
readelf -s main.o
```

```asm
Symbol table '.symtab' contains 13 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS main.c
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 .data
     4: 0000000000000000     0 SECTION LOCAL  DEFAULT    4 .bss
     5: 0000000000000000     0 SECTION LOCAL  DEFAULT    5 .rodata
     6: 0000000000000004     4 OBJECT  LOCAL  DEFAULT    3 a.1
     7: 0000000000000004     4 OBJECT  LOCAL  DEFAULT    4 b.0
     8: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    3 count
     9: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    4 value
    10: 0000000000000000    43 FUNC    GLOBAL DEFAULT    1 func
    11: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND printf
    12: 000000000000002b    52 FUNC    GLOBAL DEFAULT    1 main
```

比如func和main函数他们的类型是`FUNC`

Ndx表示section的索引（比如Printf不在main.c中，Nds是UND undefine未定义）

Value表示相对于当前section(比如.text section)起始位置的偏移量【16进制】， Size表示所占的字节数

## 对ELF示例解析

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


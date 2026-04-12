# gdb

源程序：`~/gdb/src/test.c`

## 程序调试

在main加入断点后r：

```bash
Starting program: /home/jason/gdb/src/test
Downloading separate debug info for system-supplied DSO at 0x7ffff7fc3000
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, main () at test.c:15
15      int main() {
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
─────────────────────────────[ LAST SIGNAL ]──────────────────────────────
Breakpoint hit at 0x5555555551a5
──────────[ REGISTERS / show-flags off / show-compact-regs off ]──────────
 RAX  0x555555555199 (main) ◂— endbr64
 RBX  0x7fffffffdd48 —▸ 0x7fffffffe047 ◂— '/home/jason/gdb/src/test'
 RCX  0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
 RDX  0x7fffffffdd58 —▸ 0x7fffffffe060 ◂— 'SHELL=/bin/bash'
 RDI  1
 RSI  0x7fffffffdd48 —▸ 0x7fffffffe047 ◂— '/home/jason/gdb/src/test'
 R8   0
 R9   0x7ffff7fca380 (_dl_fini) ◂— endbr64
 R10  0x7fffffffd940 ◂— 0x800000
 R13  0
 R14  0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
 R15  0x7ffff7ffd000 (_rtld_global) —▸ 0x7ffff7ffe2e0 —▸ 0x555555554000 ◂— 0x10102464c457f
 RBP  0x7fffffffdc20 —▸ 0x7fffffffdcc0 —▸ 0x7fffffffdd20 ◂— 0
 RSP  0x7fffffffdbe0 ◂— 0
 RIP  0x5555555551a5 (main+12) ◂— mov rax, qword ptr fs:[0x28]
───────────────────[ DISASM / x86-64 / set emulate on ]───────────────────
b► 0x5555555551a5 <main+12>    mov    rax, qword ptr fs:[0x28]               RAX, [0x7ffff7fb4768] => 0x2d5550d331cfaf00
   0x5555555551ae <main+21>    mov    qword ptr [rbp - 8], rax               [0x7fffffffdc18] <= 0x2d5550d331cfaf00
   0x5555555551b2 <main+25>    xor    eax, eax                               EAX => 0
   0x5555555551b4 <main+27>    mov    dword ptr [rbp - 0x14], 0x11223344     [{my_array}] <= 0x11223344
   0x5555555551bb <main+34>    mov    dword ptr [rbp - 0x10], 0x55667788     [{my_array+0x4}] <= 0x55667788
   0x5555555551c2 <main+41>    mov    dword ptr [rbp - 0xc], 0x99aabbcc      [{my_array+0x8}] <= 0x99aabbcc
   0x5555555551c9 <main+48>    mov    dword ptr [rbp - 0x30], 0xdeadc0de     [{node}] <= 0xdeadc0de
   0x5555555551d0 <main+55>    mov    byte ptr [rbp - 0x2c], 0x41            [{node+0x4}] <= 0x41
   0x5555555551d4 <main+59>    lea    rax, [rbp - 0x14]                      RAX => 0x7fffffffdc0c ◂— 0x5566778811223344
   0x5555555551d8 <main+63>    add    rax, 4                                 RAX => 0x7fffffffdc10 {my_array+0x4} (0x7fffffffdc0c + 0x4)
   0x5555555551dc <main+67>    mov    qword ptr [rbp - 0x28], rax            [{node+0x8}] <= 0x7fffffffdc10 ◂— 0x99aabbcc55667788
────────────────────────────[ SOURCE (CODE) ]─────────────────────────────
In file: /home/jason/gdb/src/test.c:15
    9
   10 int mystery_recursive(int n) {
   11     if (n <= 1) return 1;
   12     return n + mystery_recursive(n - 1); // 简单的递归累加
   13 }
   14
 ► 15 int main() {
   16     int my_array[3] = {0x11223344, 0x55667788, 0x99AABBCC};
   17
   18     struct Node node;
   19     node.id = 0xDEADC0DE;
   20     node.tag = 'A';    // ASCII 0x41
   21     node.data_ptr = &my_array[1]; // 指向数组的第二个元素 (0x55667788)
   22
────────────────────────────────[ STACK ]─────────────────────────────────
00:0000│ rsp 0x7fffffffdbe0 ◂— 0
... ↓        4 skipped
05:0028│-018 0x7fffffffdc08 —▸ 0x7ffff7fe5af0 (dl_main) ◂— endbr64
06:0030│-010 0x7fffffffdc10 {my_array+0x4} —▸ 0x7fffffffdd00 —▸ 0x555555555080 (_start) ◂— endbr64
07:0038│-008 0x7fffffffdc18 —▸ 0x7fffffffdd48 —▸ 0x7fffffffe047 ◂— '/home/jason/gdb/src/test'
──────────────────────────────[ BACKTRACE ]───────────────────────────────
 ► 0   0x5555555551a5 main+12
   1   0x7ffff7c2a1ca __libc_start_call_main+122
   2   0x7ffff7c2a28b __libc_start_main+139
   3   0x5555555550a5 _start+37
```

### 当前状态

#### REGISTERS

首先是RAX累加寄存器：现在是main的地址，后期i++运算中将会作为临时中转站

其次，RIP目前是在main+12的位置，也就是下一条要执行的命令，发现有`fs:[0x28]`也就是用到了金丝雀（Canary）保护栈

当运行 gcc 编译程序时，编译器（如 GCC 或 Clang）会默认开启一个插件：Stack Guard（堆栈保护器）。编译器在扫描你的代码时，发现满足以下任一条件，就会强制插入 Canary 代码：

- 局部变量包含数组（无论大小，my_array[3]中标了）。
- 局部变量包含结构体（struct Node node中标了）。
- 使用了`alloca`动态分配栈空间。
- 局部变量的地址被取走（执行了&my_array[1]）。

在编译器看来，只要用了数组或结构体，就有可能发生越界访问（Buffer Overflow）。为了防止`my_array`被写多了导致覆盖掉main函数的返回地址，它必须在栈帧里放一个“哨兵”。

对于Canary结尾00设计： `0x2d5550d331cfaf00`最后两位十六进制是 00（即 1 个字节的 NULL）。为了防止“字符串溢出”

C中很多函数如（strcpy, gets, scanf("%s", ...)）在处理字符串时，都有一个共同特性：遇到 \0 (ASCII 0) 就会停止拷贝。

- 如果没有00：假设攻击者想利用 printf 或某个漏洞把栈里的数据“泄露”出来。如果 Canary 中间没有 00，printf("%s", buffer) 可能会顺着 buffer 一直读下去，把 Canary 的值也打印到屏幕上。有了这个值，攻击者就能伪造一个一模一样的 Canary 来绕过检查。
- 有了 00（截断作用）：
  - 防止泄露：当程序尝试读取或打印这个字符串时，读到 Canary 的第一个字节 00 就以为字符串结束了（小端存储），从而保护了后面 7 个字节的随机数不被泄露。
  - 防止绕过：攻击者如果想通过溢出来覆盖返回地址，他必须先覆盖 Canary。但由于大多数溢出是靠字符串函数实现的，这些函数遇到 00 就停了。如果攻击者想往 Canary 后面写数据，他的 payload 里必须包含 00，这往往会导致拷贝提前终止。

##### “哨兵”位置

```bash
0x5555555551ae <main+21>    mov    qword ptr [rbp - 8], rax
```

可以看到将之前Canary的随机数放入了栈中，RBP-8恰好是栈帧中最靠近返回地址的位置。如果有人想通过溢出`my_array`改写返回地址，必须先横扫过rbp-8位置，等函数结束后，编译器会检查该位置值是否变动。

然后`0x5555555551b2 <main+25>    xor    eax, eax                               EAX => 0`是做清零操作，因为随机数已经在栈中了，把eax清零就是告诉后面的指令不再依赖eax之前的值了，有利于指令乱序执行。

#### STACK

```bash
07:0038│-008 0x7fffffffdc18 ◂— 0x2d5550d331cfaf00
```

07就是第7个条目，举例RSP偏移量是0x38，-008是距离RBP的偏移量。

```bash
pwndbg> x/3wx &my_array
0x7fffffffdc0c: 0x11223344      0x55667788      0x99aabbcc
```

```bash
0x5555555551d4 <main+59>    lea    rax, [rbp - 0x14]   RAX => 0x7fffffffdc0c ◂— 0x5566778811223344
```

这里为什么rbp-0x14是数组起始地址：首先rbp-0x8是Canary占了8B（0x2d5550d331cfaf00）。数组当前三个元素，每个元素是int类型，所以需要$3 \times 4 = 12B$，如果紧贴着Canary放，数组结尾在`rbp-0x08-0x0c = rbp-0x14`（也就是数组首元素在低地址，这是为了对抗缓冲区溢出攻击）

- 为了安全，数组末尾必须紧跟着Canary和返回地址，这样一旦数组溢出，就会撞上返回地址。为了不让第一个元素直接压在返回地址上，必须把第一个元素放到离RBP最远的地方

#### Struct Node

```c
struct Node {
    int id;           // 4字节
    char tag;         // 1字节
    // 这里会有 3字节 的 Padding (对齐)
    int *data_ptr;    // 8字节指针
};
```

逻辑上是4+1+8=13B，实际为了对齐会是$4+4+8=16B=2\times8B$。

观察STACK：

```bash
02:0010│-030 0x7fffffffdbf0 {node} ◂— 0x41deadc0de
03:0018│-028 0x7fffffffdbf8 {node+0x8} —▸ 0x7fffffffdc10 {my_array+0x4} ◂— 0x99aabbcc55667788
...
06:0030│ rax 0x7fffffffdc10 {my_array+0x4} ◂— 0x99aabbcc55667788
```

- `de ad c0 de`是`node.id`；`41`是`node.tag == 'A'`；实际上还隐藏了`00 00 00`Padding
- 对于`node.data_ptr`这8B的空间存的是地址`0x7fffffffdbf8 + 0x8 == 0x7fffffffdc10`而这个地址指向的就是`my_array+0x04 == my_array[1]`。

所以实际情况是：(小端存储)

```bash
pwndbg> x/16bx &node
0x7fffffffdbf0: 0xde    0xc0    0xad    0xde    0x41    0x00    0x00    0x00
0x7fffffffdbf8: 0x10    0xdc    0xff    0xff    0xff    0x7f    0x00    0x00
```

### 递归函数调用过程

执行

```bash
0x5555555551e5 <main+76>    call   mystery_recursive           <mystery_recursive>
```

当前情况：

#### STACK

```bash
00:0000│ rsp 0x7fffffffdbd8 —▸ 0x5555555551ea (main+81) ◂— mov dword ptr [rbp - 0x34], eax
```

执行完call指令后CPU自动就把原本下一条指令（RIP）的地址压入栈中，RSP的地址也顺势-8

#### REGISTER

- RIP内容顺势变为将要执行的第一条指令的地址。
- RDI为3也就是之前传递的第一个参数
- RSP，RBP还是之前的，还没来及更改后续

    ```bash
    0x55555555516d <mystery_recursive+4>     push   rbp
    0x55555555516e <mystery_recursive+5>     mov    rbp, rsp
    ```

    执行完这两条指令后结果为：

    ```bash
    00:0000│ rbp rsp 0x7fffffffdbd0 —▸ 0x7fffffffdc20 —▸ 0x7fffffffdcc0 —▸ 0x7fffffffdd20 ◂— 0
    ```

    可以看到RBP和RSP重新定位，并把之前的RBP存入栈中保护起来

```bash
0x555555555171 <mystery_recursive+8>     sub    rsp, 0x10
0x555555555175 <mystery_recursive+12>    mov    dword ptr [rbp - 4], edi     [{n}] <= 3
```

让RSP再向下拉开16B。由于函数存在一个参数int n，虽然是通过EDI传过来的，但在非优化的调试模式下，编译器为后续方便调试和处理，习惯性把n写回到栈中。

#### 递归的汇编

```bash
b► 0x555555555178 <mystery_recursive+15>    cmp    dword ptr [rbp - 4], 1       3 - 1     EFLAGS => 0x202 [ cf pf af zf sf IF df of iopl:00 ac ]
   0x55555555517c <mystery_recursive+19>  ✔ jg     mystery_recursive+28        <mystery_recursive+28>
    ↓
   0x555555555185 <mystery_recursive+28>    mov    eax, dword ptr [rbp - 4]     EAX, [{n}] => 3
   0x555555555188 <mystery_recursive+31>    sub    eax, 1                       EAX => 2 (3 - 1)
   0x55555555518b <mystery_recursive+34>    mov    edi, eax                     EDI => 2
   0x55555555518d <mystery_recursive+36>    call   mystery_recursive           <mystery_recursive>
```

现在是CPU从内存`[rbp-4]`取3和1做减法比较，设置标志位EFLAGS(IF=1表示允许中断，大部分程序调试的时候这个都是开的，重要的CF, ZF, SF是0)。

对于`EFLAGS => 0x202`是这个寄存器的十六进制原始数值，0x202 转换成二进制是 0010 0000 0010。其中第 1 位（始终为 1）和第 9 位（IF 中断使能，默认开启）是 1，加起来就是 0x202。

##### 递归出口

```bash
 ► 0x555555555175 <mystery_recursive+12>    mov    dword ptr [rbp - 4], edi     [{n}] <= 1
b+ 0x555555555178 <mystery_recursive+15>    cmp    dword ptr [rbp - 4], 1       1 - 1     EFLAGS => 0x246 [ cf PF af ZF sf IF df of iopl:00 ac ]
```

此时PF ZF IF == 1

```bash
b+ 0x555555555178 <mystery_recursive+15>    cmp    dword ptr [rbp - 4], 1       1 - 1     EFLAGS => 0x246 [ cf PF af ZF sf IF df of iopl:00 ac ]
   0x55555555517c <mystery_recursive+19>  ✘ jg     mystery_recursive+28        <mystery_recursive+28>

   0x55555555517e <mystery_recursive+21>    mov    eax, 1                       EAX => 1
   0x555555555183 <mystery_recursive+26>    jmp    mystery_recursive+46        <mystery_recursive+46>
    ↓
 ► 0x555555555197 <mystery_recursive+46>    leave
   0x555555555198 <mystery_recursive+47>    ret                                <mystery_recursive+41>
    ↓
   0x555555555192 <mystery_recursive+41>    mov    edx, dword ptr [rbp - 4]     EDX, [0x7fffffffdbac] => 2
   0x555555555195 <mystery_recursive+44>    add    eax, edx                     EAX => 3 (1 + 2)
   0x555555555197 <mystery_recursive+46>    leave
   0x555555555198 <mystery_recursive+47>    ret 
```

leave：实际上做了两件事：

- mov rsp, rbp (把 RSP 提到地基处，丢弃掉那 16 字节局部变量)
- pop rbp (把 RBP 弹回上一层的地基，即 ...dbb0)

ret：从栈顶弹出返回地址，瞬间把 RIP 扔回上一层 call 的后一行。

执行完leave的情况：

`rsp 0x7fffffffdb80 ◂— 0x800`变成了` rsp 0x7fffffffdb98 —▸ 0x555555555192 (mystery_recursive+41) ◂— mov edx, dword ptr [rbp - 4]`把rbp的内容给了rsp，最开始mov指令确实rsp是...90，后面`pop rbp`后要填补空白的8B，所以就变成了...98。

`rbp 0x7fffffffdb90 —▸ 0x7fffffffdbb0 —▸ 0x7fffffffdbd0 —▸ 0x7fffffffdc20 —▸ 0x7fffffffdcc0`变成了` rbp 0x7fffffffdbb0 —▸ 0x7fffffffdbd0 —▸ 0x7fffffffdc20 —▸ 0x7fffffffdcc0 —▸ 0x7fffffffdd20`可以看出是pop出栈了。

### 返回值拿到

```bash
0x5555555551ea <main+81>                 mov    dword ptr [rbp - 0x34], eax     [{result}] <= 6
```

递归的RAX=6被存进了main预留变量为result的空间（rbp-0x24）

在栈中是：

```bash
00:0000│ rsp 0x7fffffffdbe0 ◂— 0
01:0008│-038 0x7fffffffdbe8 ◂— 0x600000000
```

后面`mov    eax, dword ptr [rbp - 0x34]`才把之前栈中数据拿回到EAX中。

为什么两步：

为了调试准确：如果不经过 [rbp-0x34] 存一下，在 GDB 里你输入 print result 就会找不到这个值。编译器为了让你能调试，必须把值写进内存。

寄存器污染保护：在复杂的程序里，EAX 极其忙碌。编译器不敢保证从函数返回到调用 printf 之间，EAX 会不会被其他指令改掉。最稳妥的办法就是：先存入内存（安全区），要用的时候再取出来。

如果开`gcc -O2`优化可能就直接了。
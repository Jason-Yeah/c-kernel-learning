# 1-1 一条机器指令如何执行

[动画入口](instruction-cycle.html) · [第1章目录](index.html) · [完整C](../experiments/instruction-cycle.c) · [原始JSON](../traces/instruction-cycle.json) · [原始GDB](../traces/instruction-cycle-gdb.txt)

## 为什么按计划先做这一项

原计划按章节列出候选，没有额外优先级承诺。“源码到运行”已在独立OS导读覆盖，故本次补第1章首个未覆盖项：取指、译码、执行、写回和后继RIP。下一节[第2章字节序](../chapter2/byte-order.html)现已完成，进一步区分位权与内存字节排列。

## 完整实验

instruction-cycle.c 是安全的Linux x86-64单线程程序：全局inputs初始化为3和2，全局observed按C静态存储期零初始化；calculate读入、相加、写observed并返回。无越界、无陌生用户程序执行，仅禁用本进程core，保留宿主ASLR与系统保护。

```bash
npm run capture:cycle   # 真正GCC构建与GDB逐指令采集
npm run build
npm run test:cycle      # 原始ELF/GDB与画面独立核对
```

本次18个真实指令停点、9个命名事件，实际stdout为inputs=3,2 observed=5 return=5，程序退出0。源码／ELF SHA-256、完整参数、工具、ASLR与时间随数据保存。换编译器或重采时地址可变化，渲染器不回填手写地址。

## 一条MOV中的取指与译码

代码和数据同属这个进程的虚拟地址空间；分镜中分别放大，不代表两份物理RAM。

GDB看见RIP处的实际代码字节；本次选定的输入MOV长6字节，形如8B 05 disp32。内存按高地址在上竖向排列，而指令字节串按从低地址到高地址的实际顺序展示。

8B说明r/m32到r32，ModR/M=05在本例指定EAX及RIP-relative寻址；有符号disp32按小端解释。有效地址为“后继RIP + disp32”，不是“当前RIP + disp32”，结果与实际inputs[0]地址一致；执行后EAX实际读到3。字段拆解可与软件反汇编核对，但不是捕获了CPU内部译码器。

## 执行与写回

为了不把一行C误说成一条机器指令，动画明确略过若干实际局部读写后，跳到ADD前的真实停点：EAX=2，EDX=3；执行后EAX/RAX=5。前态和后态是同一寄存器的两张快照，并不是CPU有两个EAX。

后续真实MOV把EAX写到observed。输入3/2始终不变，输出只在store后由0变5。四字节作为一个组统一提交；移动字节只是解释数据去向，不声称实测总线按字节逐个写入。小端字节按实际地址排列，数组容量不变化。

## 下一条指令在哪里

本例顺序指令满足RIP_next=RIP+真实指令长度，不是统一加1。RET是对照：它使用入口栈顶保存的8B返回地址，实际回到main后继指令，RSP恢复；整数返回值5仍在RAX里。返回地址、当前RIP和整数返回值是三个不同概念。

## 连续衔接

FETCH的原代码字节列和RIP直接保留到DECODE，不再清空后重画；MOV完成时才更新实际RIP/EAX。ADD写回的结果视图继续作为STORE的数据源，再以同一物理RAX的视图保留到RET与总结。EAX和RAX是位宽别名，不是两个独立寄存器；略过的实际中间指令仍明确说明，返回地址不与整数返回值合并。原96秒、18个实际停点与各提交时刻不变。

## 观测边界

GDB记录的是指令边界上的架构状态。取指、译码、ALU、控制令牌、箭头和教学速度是解释模型，省略Cache/TLB、流水线、乱序和推测，不声称一条x86指令对应一个时钟或按动画节奏运行。

页面实验台保留每个停点完整C、原样命令、当前与下一次单步的前后状态。定位按钮明确为对应机制讲解，未对应的中间停点保留原始记录；网页不会运行C或实时控制GDB。可下载受控[ELF](../traces/instruction-cycle/program.elf)和[反汇编](../traces/instruction-cycle/disassembly.txt)核对代码字节，不自动执行。

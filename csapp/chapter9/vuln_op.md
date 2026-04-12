# 攻击原理

## 编译修改
现有的GCC11+会有安全机制，不可使用gets，导致编译错误，现在使用如下命令会使得GCC关闭安全机制，使用旧的标准
```bash
gcc -fno-stack-protector -no-pie -Wno-implicit-function-declaration -o vuln vuln.c
```

- -fno-stack-protector: 禁用“栈金丝雀”保护，允许我们覆盖返回地址。

- -no-pie: 禁用位置无关可执行文件，确保 secret_backdoor 的地址固定，方便我们查找。

- -Wno-implicit-function-declaration: 忽略 gets 声明缺失的警告，将其视为普通警告而非错误。

## 核心原理

在 C 语言中，函数调用时会在内存中创建一个栈帧（Stack Frame）。 它的布局是固定的：

buffer[16]：放在最下面（低地址）。

Saved RBP：中间的 8 字节，保存上一个函数的基准位置。

Return Address（返回地址）：放在最上面（高地址）。 程序执行完 `echo_input` 后，CPU 会读取这个地址，跳回 main 函数继续执行。

攻击逻辑：gets 函数不检查长度。 输入了 24 字节的垃圾（16 字节填满 buffer + 8 字节填满 RBP），然后紧接着输入的 8 字节地址刚好覆盖了原本的“返回地址”。 当函数结束要“回家”时，被指引到了 `secret_backdoor` 的家。

## 执行过程

### 探测目标
```bash
nm vuln | grep secret_backdoor
```
- nm：列出可执行文件中的符号表（函数名、变量名及其对应的内存地址）。
- 作用：确定后门函数在内存中的“经纬度”（例如0x401196）。

### 制造炸弹
```python
python3 -c "import sys; sys.stdout.buffer.write(b'A'*24 + b'\x96\x11\x40\x00\x00\x00\x00\x00')" > payload
```
- b'A'*24：精准的填充物。前 16 字节塞满数组，后 8 字节塞满 RBP 寄存器的备份槽。
- b'\x96\x11\x40...'：这就是你要注入的伪造返回地址。
- 小端序反写：x86 架构在内存中存储地址是“低位在前”。所以 0x401196 必须写成 96 11 40。
- `> payload`：将这段二进制数据保存到文件，因为这些特殊字符没法通过键盘直接打出来。

第三步：引爆炸弹
```bash
cat payload | ./vuln
```
- cat payload：把构造好的二进制串吐出来。
- | (管道)：把上一个命令的输出，直接灌进 ./vuln 的标准输入（即 gets 接收的地方）。
- 效果：gets 毫无防备地接收了所有数据，完成了对返回地址的篡改。
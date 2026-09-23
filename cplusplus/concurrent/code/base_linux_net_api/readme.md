# Linux 网络基础 API 配套实验（C17 / C++20）

对应[主教程 5.1～5.12](../../doc/base_linux_net_api.md)。所有网络实验使用 IPv4 回环地址 `127.0.0.1`，不需要外部服务器。解析实验支持显示 IPv4/IPv6，但通信示例本身只实现 IPv4。

原书对照范围为所附 PDF 第 168～222 页。主教程逐节标出页码及旧表述需要修正之处。本目录代码为重新编写的学习示例；书中的 backlog 堆积、断网实验和 tcpdump 抓包结果没有包含在自动测试中。

## 1. 文件与学习目标

| 文件 | 内容 | 对应章节 |
|---|---|---|
| `tcp_c17.c` | 原始字节流 TCP echo 服务端，显式错误路径与资源清理 | 5.1～5.8、5.10、5.11 |
| `net.hpp` | fd 独占所有权、地址辅助函数、完整读写、长度前缀协议 | C++11～20 与 5.7、5.8 |
| `tcp_cpp20.cpp` | 带长度前缀的 TCP 服务端/客户端、半关闭、对端查询 | 5.1～5.8、5.10、5.11 |
| `udp_cpp20.cpp` | sendto/recvfrom 与 connected UDP send/recv 对照 | 5.6、5.8.2 |
| `api_lab.cpp` | 地址解析、选项读取、分散聚集 I/O、带外标记 | 5.8.3～5.12 |
| `smoke_test.py` | 真实回环通信及异常报文验证 | 全套自检 |
| `Makefile` | 编译、测试、清理 | 工程入口 |

这些程序是有意保持简单的阻塞实验：TCP 服务端接待一个连接后退出，UDP 服务端回显一个数据报后退出。它们不是线程池或 epoll 高并发服务器。TCP 演示没有配置完整超时，手动运行时可以用 Ctrl+C 结束等待。

## 2. 编译与自动检查

需要 Linux、支持 C++20 的编译器和标准库、GNU Make；自动测试另需 Python 3。已在 GCC/G++ 13.3.0、C17/C++20 模式下编译验证。

```bash
cd /home/jason/study/ck_study/cplusplus/concurrent/code/base_linux_net_api
make
make check
```

`make` 是根据依赖生成程序的工具，不是 C++ 语法。`-std=c17` 或 `-std=c++20` 选择语言标准；`-Wall -Wextra -Wpedantic` 开启常用警告；`-O2` 开启优化。`.hpp` 是头文件，被 `.cpp` 包含，不单独运行。示例只依赖系统库和标准库，无需安装 Boost。

自动测试使用端口 0，请内核分配空闲端口；读到服务端的 `PORT 数字` 后才连接，不靠固定 sleep 猜启动时间。启动、客户端和子进程退出都有等待限制，失败会清理测试进程。

已验证：C17 二进制回显、C++20 客户端与半关闭、空帧、含零字节帧、256 KiB 帧、分段提交消息头、多帧连续发送、头/正文截断、超大长度拒绝、UDP 普通/空报文、解析、选项、UDP 截断及 OOB 标记。测试不保证内核一定把分段提交映射为分段接收，也不声称模拟了拥塞和丢包。

受限容器可能在 `socket()` 时出现 `Operation not permitted`，这与编译错误不同，需要在允许本机 socket 的环境运行。清理生成的四个可执行文件用 `make clean`；它们已加入本目录 `.gitignore`。

## 3. C17：先看最直接的生命周期

终端 A：

```bash
./tcp_c17 9000
```

输出 `PORT 9000` 后等待客户端。终端 B 使用 Python 的系统 socket 封装发送一个原始字节流：

```bash
python3 - <<'PY'
import socket
with socket.create_connection(('127.0.0.1', 9000), timeout=3) as s:
    s.sendall(b'hello C17\n')
    s.shutdown(socket.SHUT_WR)
    chunks = []
    while True:
        data = s.recv(4096)
        if not data:
            break
        chunks.append(data)
    print(b''.join(chunks).decode(), end='')
PY
```

预期终端 B 输出 `hello C17`，终端 A 正常结束。Python 只是便于发送测试数据，不是本教程要求学习的网络语言。

### 按代码执行顺序理解

1. `listener = -1, peer = -1` 表示尚无资源；只有非负 fd 才关闭。
2. `strtoul` 解析端口，并检查范围和输入尾部，防止端口转换时悄悄截断。
3. `socket` 创建端点，`setsockopt` 设置复用选项，`sockaddr_in` 描述本地地址。
4. `bind` 指定地址，`listen` 建立监听状态，`getsockname` 查询实际端口。
5. `accept` 得到已连接 fd。这里保留传统 accept 写法；示例不执行 exec，现代 Linux 程序可参考 C++ 示例使用 accept4 设置 CLOEXEC。
6. 外层 `recv` 循环按实际长度读。内部 `send` 循环直到该段全部回显，使用 `offset` 指向尚未发出的部分。
7. 收到 EOF 后进入 `cleanup`，与所有错误路径共用资源回收。

`goto cleanup` 只跳到当前函数的清理标签，并不是跳回主循环。每个失败点先 `perror`，避免清理调用覆盖需要报告的错误。`EXIT_SUCCESS/EXIT_FAILURE` 是表示程序退出结果的标准常量。

C 程序不解释消息内容，所以能回显任意字节。它没有应用层长度协议，不能用来演示“服务器理解了一整条业务消息”。

## 4. C++20：分帧与半关闭

终端 A：

```bash
./tcp_cpp20 server 9001
```

终端 B：

```bash
./tcp_cpp20 client 9001 '你好，Linux socket'
```

客户端打印相同文本；服务端在标准错误中打印对端的回环地址和临时端口，然后两边退出。测试空消息时传 `''`，客户端会打印空行。

### net.hpp 如何阅读

先读 `unique_fd`，再读地址辅助函数，最后读 `send_all → recv_exact → send_frame/recv_frame`。

- `explicit` 防止整数不经说明就隐式变成 fd 所有者。
- `= delete` 禁止复制构造与复制赋值，避免两个对象关闭同一个 fd。
- `unique_fd&&` 是移动接口；`std::exchange(other.fd_, -1)` 取走旧值并把来源清空。
- `noexcept` 表示移动和析构等操作不会传播异常；析构只做一次 close。
- `get()` 借出编号，调用方不能擅自关闭；此封装不提供共享所有权。
- `check` 检查返回 -1 的系统调用，`fail` 将当前 errno 封装成 `std::system_error`。
- `port_number` 校验十进制端口，`loopback` 构造地址，`bind_local/bound_port` 减少重复代码。
- `send_all` 的 span 每成功发送 n 字节便缩短 n 字节；它只适用于本例的阻塞 TCP。
- `recv_exact` 直到目标字段读满；字段尚未读任何字节时 EOF 返回 false，中途 EOF 抛异常。
- `send_frame` 先发网络字节序的四字节长度，再发正文。
- `recv_frame` 检查长度不超过 1 MiB，然后分配正文。正文长度为零时直接成功；非零正文尚未开始就 EOF 也算错误。

`reinterpret_cast<char*>(&len)` 把整数的对象表示作为字节访问，但这个整数已经用 htonl 转成协议要求的表示；这不等于“可以直接传任何 C++ 对象”。

### 一次往返的时序

```text
客户端                       服务端
connect -------------------> 内核握手，accept 返回 peer
send_frame(length + body) --> recv_frame
shutdown(SHUT_WR) ----------> 读完请求后可观察 EOF
recv_frame <---------------- send_frame 原样回显
继续读到 EOF <-------------- shutdown(SHUT_WR)
析构 close                   析构 close peer 和 listener
```

客户端发完请求后保留读取方向，服务端收到 EOF 后结束循环。协议本身支持同一连接多个帧，命令行客户端为简单起见只发送一个；自动测试验证多帧。

C17 与 C++20 的协议处理不同：C17 是不解释内容的原始回显，C++20 服务端需要四字节长度头。普通文本客户端不能直接向 C++20 服务端发送一行文本，否则前四字节会被当成长度。C++20 客户端偶尔能与原始 echo 互通，因为原始 echo 连头也照样回传，这不代表两端都在解析同一种协议。

## 5. UDP：一份报文就是一个边界

终端 A：

```bash
./udp_cpp20 server 9002
```

终端 B：

```bash
./udp_cpp20 client 9002 'hello UDP'
```

每次重试前重新启动服务端。空报文用 `./udp_cpp20 client 9002 ''`，它是合法数据，不是 TCP 式 EOF。

服务端调用 `recvfrom` 同时得到正文、来源地址与来源地址长度，再把同一份报文用 `sendto` 发回。客户端先 UDP connect，然后使用 send/recv；这个 connect 没有握手，只是设置默认对端。客户端设置 3 秒接收超时，演示网络无响应时必须有结束等待的办法。

缓冲区是 `std::array<char, 65536>`，大小编译时确定，避免本例 IPv4 UDP 数据报因用户缓冲区太小被截断。发送不使用 send_all，因为分次发送会变成多份数据报。`std::cout.write` 使用实际接收长度，不依赖零结尾字符串。

## 6. api_lab：四个小实验

### 地址解析与服务数据库

```bash
./api_lab resolve localhost 80
./api_lab resolve 127.0.0.1 80
./api_lab resolve ::1 80
```

候选数量、顺序由系统配置决定；显示 IPv6 结果不等于 IPv6 服务正在监听。`getaddrinfo` 返回链表，`unique_ptr<addrinfo, decltype(&freeaddrinfo)>` 表示“用 freeaddrinfo 释放的智能指针”；`decltype` 从表达式推导类型，这里得到删除函数指针类型。

`for (auto p = list.get(); p; p = p->ai_next)` 沿链表逐项访问。每项用 getnameinfo 加数字标志格式化，最后演示旧 `getservbyname("http", "tcp")`。为聚焦 API，实验将 IPv6 输出为简单地址与端口串；正式日志应加方括号区分端口。

### 查询 socket 选项

```bash
./api_lab options
```

先设置发送缓冲区请求值 32768，再查询多个选项，最后查询 linger 结构体。Linux 常显示 SO_SNDBUF 为 65536，但系统限制可能改变结果，不应把该数字写成测试断言。`option 数字` 是当前平台头文件中的选项编号；输出顺序为 SO_REUSEADDR、SO_SNDBUF、SO_RCVBUF、SO_RCVLOWAT、SO_SNDLOWAT。

这里只查询发送低水位，不尝试设置 Linux 不支持更改的选项。linger 默认常为关闭；实验不使用强制复位关闭来破坏数据交付。

### sendmsg/recvmsg 与截断

```bash
./api_lab msg
```

预期：

```text
copied=7 truncated=1 data=HEAD:bo
```

发送端的两个 iovec 分别指向 `HEAD:` 和 `body`，一次发送 9 字节。接收端两个 iovec 分别只有 3、4 字节，共 7 字节，接收后 `msg_flags` 带 MSG_TRUNC。剩余 `dy` 已丢弃，不会留给下次接收。代码仅输出实际写入的字节，防止把未接收空间误当内容。

这里 recvmsg 第三个参数为 0，因此返回已复制长度 7；Linux UDP 如果输入 flags 也指定 MSG_TRUNC，可以要求返回原始数据报长度，届时不能按该返回值直接访问只有 7 字节的缓冲区。

### 带外标记

```bash
./api_lab oob
```

本进程先建立一对回环 TCP 端点：客户端 connect 可在应用 accept 前完成，因为握手由内核处理。发送总量很小，不需要为实验额外创建线程。

接收端设置 SO_OOBINLINE，发送端依次送 `abc`、OOB `!`、`xyz`。预期逐字节显示，其中：

```text
mark=1 byte=!
```

其余普通字符通常 mark=0。循环先等待 POLLIN 再查询 sockatmark、读取一个字节，避免提前查询位置后又阻塞而混淆观察。这里 inline 紧急字节按普通数据读取，所以不使用 MSG_OOB 接收。poll 的 3 秒只是保护实验等待；poll 的 `POLLPRI` 用途及非 inline 读取见主教程。

## 7. 建议练习

1. 将 C17 接收缓冲区改成 3 字节，观察它仍能回显长字符串，解释为什么。
2. 把 C++20 客户端文本改为空字符串，区分“长度为零的帧”和 TCP EOF。
3. 把 UDP 实验接收空间扩大到 9 字节，观察截断标志消失。
4. 用端口 0 启动服务器，根据打印的端口手动连接，理解 getsockname 的作用。
5. 用主教程的 ss/strace 命令观察 listener 与 peer，按 API 顺序对照代码。

不要先急着加入多线程。先能解释每个 fd 的所有者、每个返回值的含义、每个缓冲区的有效长度，再扩展到非阻塞服务器。

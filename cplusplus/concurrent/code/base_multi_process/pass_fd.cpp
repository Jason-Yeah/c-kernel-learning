#include "process_support.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <vector>

static int send_fd(int socket, int fd)
{
    char tag = 'F';
    iovec data{&tag, 1};
    alignas(cmsghdr) unsigned char control[CMSG_SPACE(sizeof(int))]{};
    msghdr message{};
    message.msg_iov = &data;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);
    cmsghdr *header = CMSG_FIRSTHDR(&message);
    header->cmsg_level = SOL_SOCKET;
    header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(header), &fd, sizeof(fd));
    ssize_t sent;
    do
    {
        sent = ::sendmsg(socket, &message, MSG_NOSIGNAL);
    } while (sent < 0 && errno == EINTR);
    return sent == 1 ? 0 : 1;
}

static UniqueFd receive_fd(int socket)
{
    char tag = 0;
    iovec data{&tag, 1};
    alignas(cmsghdr) unsigned char control[CMSG_SPACE(sizeof(int))]{};
    msghdr message{};
    message.msg_iov = &data;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);
    std::vector<UniqueFd> descriptors;
    descriptors.reserve(sizeof(control) / sizeof(int));
    ssize_t n;
    do
    {
        n = ::recvmsg(socket, &message, MSG_CMSG_CLOEXEC);
    } while (n < 0 && errno == EINTR);
    if (n < 0)
        system_fail("recvmsg");

    for (auto *h = CMSG_FIRSTHDR(&message); h; h = CMSG_NXTHDR(&message, h))
    {
        if (h->cmsg_level != SOL_SOCKET || h->cmsg_type != SCM_RIGHTS)
            continue;
        if (h->cmsg_len < CMSG_LEN(0))
            throw std::runtime_error("malformed control header");
        auto bytes = h->cmsg_len - CMSG_LEN(0);
        for (std::size_t offset = 0; offset + sizeof(int) <= bytes;
             offset += sizeof(int))
        {
            int fd;
            std::memcpy(&fd, CMSG_DATA(h) + offset, sizeof(fd));
            descriptors.emplace_back(fd);
        }
    }
    // 先拥有收到的 fd，再检查截断；抛异常也不会遗留已接收的 fd。
    if (n != 1 || tag != 'F' ||
        (message.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) ||
        descriptors.size() != 1)
        throw std::runtime_error(
            "expected exactly one intact SCM_RIGHTS descriptor");
    return std::move(descriptors.front());
}

int main()
try
{
    char path[] = "/tmp/ck_pass_fd_XXXXXX";
    UniqueFd file(::mkstemp(path));
    if (file.get() < 0)
        system_fail("mkstemp");
    if (::unlink(path) < 0)
        system_fail("unlink temp file");
    const char text[] = "hello via passed file descriptor\n";
    std::size_t written = 0;
    while (written < sizeof(text) - 1)
    {
        auto n =
            ::write(file.get(), text + written, sizeof(text) - 1 - written);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0)
            system_fail("write temp file");
        if (n == 0)
            throw std::runtime_error("write made no progress");
        written += static_cast<std::size_t>(n);
    }
    if (::lseek(file.get(), 0, SEEK_SET) < 0)
        system_fail("lseek");
    int pair[2];
    if (::socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0, pair) < 0)
        system_fail("socketpair");
    UniqueFd receiver(pair[0]), sender(pair[1]);
    pid_t pid = ::fork();
    if (pid < 0)
        system_fail("fork");
    if (pid == 0)
    {
        ::close(receiver.get());
        _exit(send_fd(sender.get(), file.get()));
    }
    Child child(pid);
    sender.reset();
    file.reset(); // 父进程原 fd 已关闭；将靠 SCM_RIGHTS 收到一个新的 fd。
    int status =
        child
            .wait(); // 一个小 datagram 能入队，成功后再取避免发送失败时永久等。
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        throw std::runtime_error("child failed to send descriptor");
    auto received = receive_fd(receiver.get());
    int flags = ::fcntl(received.get(), F_GETFD);
    if (flags < 0)
        system_fail("fcntl");
    std::cout << "received fd=" << received.get()
              << " CLOEXEC=" << ((flags & FD_CLOEXEC) != 0) << '\n';
    std::array<char, 128> buffer{};
    for (;;)
    {
        auto n = ::read(received.get(), buffer.data(), buffer.size());
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0)
            system_fail("read received fd");
        if (n == 0)
            break;
        std::cout.write(buffer.data(), n);
    }
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

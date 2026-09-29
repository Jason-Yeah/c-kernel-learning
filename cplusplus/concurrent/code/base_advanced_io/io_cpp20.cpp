#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <span>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>

[[noreturn]] static void system_fail(const char *what)
{
    throw std::system_error(errno, std::generic_category(), what);
}

class unique_fd
{
    int value_ = -1;

public:
    explicit unique_fd(int value = -1) noexcept : value_(value) {}
    ~unique_fd()
    {
        if (value_ >= 0)
            ::close(value_);
    }

    unique_fd(const unique_fd &) = delete;
    unique_fd &operator=(const unique_fd &) = delete;
    unique_fd(unique_fd &&other) noexcept
        : value_(std::exchange(other.value_, -1))
    {
    }
    int get() const noexcept { return value_; }
};

class read_mapping
{
    void *address_ = MAP_FAILED;
    std::size_t length_ = 0;

public:
    read_mapping(int fd, std::size_t length) : length_(length)
    {
        if (length_ != 0)
        {
            address_ = ::mmap(nullptr, length_, PROT_READ, MAP_PRIVATE, fd, 0);
            if (address_ == MAP_FAILED)
                system_fail("mmap");
        }
    }
    ~read_mapping()
    {
        if (address_ != MAP_FAILED)
            ::munmap(address_, length_);
    }
    read_mapping(const read_mapping &) = delete;
    read_mapping &operator=(const read_mapping &) = delete;

    std::span<const std::byte> bytes() const noexcept
    {
        if (length_ == 0)
            return {};
        return {static_cast<const std::byte *>(address_), length_};
    }
};

static void write_all(int fd, std::span<const std::byte> bytes)
{
    while (!bytes.empty())
    {
        auto n = ::write(fd, bytes.data(), bytes.size());
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            system_fail("write");
        }
        if (n == 0)
            throw std::runtime_error("write made no progress");
        bytes = bytes.subspan(static_cast<std::size_t>(n));
    }
}

int main(int argc, char **argv)
try
{
    if (argc != 2)
        throw std::runtime_error("usage: io_cpp20 FILE");
    unique_fd file(::open(argv[1], O_RDONLY | O_CLOEXEC));
    if (file.get() < 0)
        system_fail("open");

    struct stat info
    {
    };

    if (::fstat(file.get(), &info) < 0)
        system_fail("fstat");

    if (!S_ISREG(info.st_mode))
        throw std::runtime_error("input must be a regular file");
    if (info.st_size < 0)
        throw std::runtime_error("negative file size");

    //
    read_mapping mapped(file.get(), static_cast<std::size_t>(info.st_size));
    write_all(STDOUT_FILENO, mapped.bytes());
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

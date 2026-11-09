#include "process_support.hpp"
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <semaphore.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>

struct Shared
{
    sem_t ready;
    char text[128]; // 不放 std::string 或指向进程私有堆的裸指针。
};

struct NameOwner
{
    std::string name;
    bool linked = true;
    ~NameOwner()
    {
        if (linked)
            ::shm_unlink(name.c_str());
    }
    void unlink_now()
    {
        if (::shm_unlink(name.c_str()) < 0)
            system_fail("shm_unlink");
        linked = false;
    }
};

struct Mapping
{
    void *address;
    std::size_t size;
    ~Mapping() { ::munmap(address, size); }
};

struct SemaphoreOwner
{
    sem_t *semaphore;
    ~SemaphoreOwner() { ::sem_destroy(semaphore); }
};

int main()
try
{
    std::string name = "/ck_multi_process_" + std::to_string(::getpid());
    UniqueFd fd(::shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600));
    if (fd.get() < 0)
        system_fail("shm_open");
    NameOwner owner{name};
    owner.unlink_now(); // 通过继承通信，因此不需要留名字给第三个进程重新打开。
    if (::ftruncate(fd.get(), sizeof(Shared)) < 0)
        system_fail("ftruncate");
    void *memory = ::mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd.get(), 0);
    if (memory == MAP_FAILED)
        system_fail("mmap");
    Mapping mapping{memory, sizeof(Shared)};
    auto *shared = std::construct_at(static_cast<Shared *>(memory));
    if (::sem_init(&shared->ready, 1, 0) < 0)
        system_fail("sem_init process-shared");
    SemaphoreOwner semaphore{&shared->ready};
    fd.reset(); // 映射仍然有效。

    pid_t pid = ::fork();
    if (pid < 0)
        system_fail("fork");
    if (pid == 0)
    {
        const char text[] = "hello from POSIX shared memory";
        std::memcpy(shared->text, text, sizeof(text));
        int result = ::sem_post(&shared->ready);
        _exit(result == 0 ? 0 : 1);
    }
    Child child(pid);
    timespec deadline{};
    if (::clock_gettime(CLOCK_REALTIME, &deadline) < 0)
        system_fail("clock_gettime");
    deadline.tv_sec += 5; // 教学防挂起；POSIX sem_timedwait 使用绝对实时时钟。
    while (::sem_timedwait(&shared->ready, &deadline) < 0)
    {
        if (errno != EINTR)
            system_fail("sem_timedwait");
    }
    int status = child.wait();
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        throw std::runtime_error("child failed");
    std::cout << "parent read: " << shared->text << '\n';
    // 按相反顺序：回收 child -> sem_destroy -> munmap；名字早已 unlink。
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

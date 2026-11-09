#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "process_support.hpp"
#include <spawn.h>
#include <fcntl.h>
#include <array>
#include <iostream>
#include <string>

extern char **environ;

static void check_spawn_api(int error, const char *what)
{
    if (error != 0)
        throw std::system_error(error, std::generic_category(), what);
}

class FileActions
{
public:
    posix_spawn_file_actions_t actions;
    FileActions() { check_spawn_api(::posix_spawn_file_actions_init(&actions), "file_actions_init"); }
    ~FileActions() { ::posix_spawn_file_actions_destroy(&actions); }
    FileActions(const FileActions &) = delete;
    FileActions &operator=(const FileActions &) = delete;
};

int main()
try
{
    int pipefd[2];
    if (::pipe2(pipefd, O_CLOEXEC) < 0)
        system_fail("pipe2");
    UniqueFd reader(pipefd[0]), writer(pipefd[1]);
    FileActions actions;
    check_spawn_api(::posix_spawn_file_actions_addclose(&actions.actions, reader.get()), "addclose reader");
    check_spawn_api(::posix_spawn_file_actions_adddup2(&actions.actions, writer.get(), STDOUT_FILENO), "adddup2 stdout");
    if (writer.get() != STDOUT_FILENO)
        check_spawn_api(::posix_spawn_file_actions_addclose(&actions.actions, writer.get()), "addclose writer");
    char program[] = "/bin/echo";
    char argument[] = "hello from posix_spawn";
    char *argv[] = {program, argument, nullptr};
    pid_t pid;
    check_spawn_api(::posix_spawn(&pid, program, &actions.actions, nullptr, argv, environ), "posix_spawn");
    Child child(pid);
    writer.reset(); // 如果父进程留着写端，读完子输出后仍无法看到 EOF。
    std::string output;
    std::array<char, 256> buffer{};
    for (;;)
    {
        auto n = ::read(reader.get(), buffer.data(), buffer.size());
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0)
            system_fail("read child stdout");
        if (n == 0)
            break;
        output.append(buffer.data(), static_cast<std::size_t>(n));
    }
    int status = child.wait();
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        throw std::runtime_error("spawned program failed");
    std::cout << "captured: " << output;
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}

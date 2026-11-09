/* Fixed, reviewed Linux x86-64 experiment. Only fault mode deliberately traps.
 * GNU asm marks exact instructions; kernel execution is not traced by GDB. */
#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int input_a = 3, input_b = 2, observed_sum;
long observed_pid, syscall_number = SYS_getpid, read_syscall_number = SYS_read;
unsigned char *guard_page;
long page_size;
unsigned char fault_value;
int fault_read_complete;
unsigned char sender_byte = 'K';

__attribute__((noinline)) static void checkpoint(unsigned phase) {
    __asm__ volatile("" : : "r"(phase) : "memory");
}
__attribute__((noinline)) static int local_add(int a, int b) {
    return a + b;
}
__attribute__((noinline)) static long raw_getpid(void) {
    long result = SYS_getpid;
    __asm__ volatile(".globl ec_syscall\nec_syscall:\nsyscall\n"
                     ".globl ec_syscall_after\nec_syscall_after:\n"
                     : "+a"(result) : : "rcx", "r11", "memory", "cc");
    return result;
}
__attribute__((noinline)) static unsigned char fault_load(const unsigned char *p) {
    unsigned char result;
    __asm__ volatile(".globl ec_fault\nec_fault:\nmovb (%%rdi), %%al\n"
                     ".globl ec_fault_after\nec_fault_after:\n"
                     : "=a"(result) : "D"(p) : "memory");
    return result;
}
static int inspect(void) {
    checkpoint(1);
    observed_sum = local_add(input_a, input_b);
    checkpoint(2);
    observed_pid = raw_getpid();
    checkpoint(3);
    printf("{\"event\":\"inspect-result\",\"pid\":%ld,\"sum\":%d}\n",
           observed_pid, observed_sum);
    return !(observed_sum == 5 && observed_pid == (long)getpid());
}
static int fault(void) {
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) return 2;
    guard_page = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (guard_page == MAP_FAILED) return 2;
    guard_page[0] = 0xA5;
    checkpoint(4); /* Only here is the protected target read by the debugger. */
    if (mprotect(guard_page, (size_t)page_size, PROT_NONE) != 0) return 2;
    fault_value = fault_load(guard_page); /* Deliberate instruction fault, own mapping. */
    fault_read_complete = 1; /* Must NOT be reached in the recorded run. */
    munmap(guard_page, (size_t)page_size);
    return 3;
}
static int pin_one_allowed_cpu(void) {
    cpu_set_t allowed, one;
    if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0) return -1;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (!CPU_ISSET(cpu, &allowed)) continue;
        CPU_ZERO(&one);
        CPU_SET(cpu, &one);
        if (sched_setaffinity(0, sizeof(one), &one) != 0) return -1;
        return cpu; /* This process and its child only; not the host mask. */
    }
    return -1;
}
static ssize_t read_one(int fd, unsigned char *p) {
    ssize_t n;
    do { n = read(fd, p, 1); } while (n < 0 && errno == EINTR);
    return n;
}
static ssize_t write_one(int fd, const unsigned char *p) {
    ssize_t n;
    do { n = write(fd, p, 1); } while (n < 0 && errno == EINTR);
    return n;
}
static int blocking(void) {
    int cpu = pin_one_allowed_cpu(), pipefd[2];
    if (cpu < 0 || pipe(pipefd) != 0) return 2;
    pid_t child = fork();
    if (child < 0) { close(pipefd[0]); close(pipefd[1]); return 2; }
    if (child == 0) {
        close(pipefd[0]);
        unsigned char release = 0;
        if (read_one(STDIN_FILENO, &release) != 1 || release != '!') _exit(4);
        printf("{\"event\":\"helper-write\",\"pid\":%ld,\"ppid\":%ld,\"cpu\":%d,\"byte\":%u}\n",
               (long)getpid(), (long)getppid(), sched_getcpu(), (unsigned)sender_byte);
        ssize_t n = write_one(pipefd[1], &sender_byte);
        close(pipefd[1]);
        _exit(n == 1 ? 0 : 5);
    }
    close(pipefd[1]);
    unsigned char received = 0xCC;
    struct rusage before, after;
    printf("{\"event\":\"blocking-ready\",\"pid\":%ld,\"child\":%ld,\"cpu\":%d,\"fd\":%d,\"buffer\":\"%p\",\"capacity\":1,\"beforeByte\":%u,\"readNumber\":%ld}\n",
           (long)getpid(), (long)child, cpu, pipefd[0], (void *)&received,
           (unsigned)received, read_syscall_number);
    if (getrusage(RUSAGE_SELF, &before) != 0) return 2;
    ssize_t n = read_one(pipefd[0], &received); /* Empty pipe: collector observes this wait. */
    if (getrusage(RUSAGE_SELF, &after) != 0) return 2;
    close(pipefd[0]);
    int status = 0;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    printf("{\"event\":\"blocking-result\",\"pid\":%ld,\"cpu\":%d,\"readResult\":%ld,\"byte\":%u,\"capacity\":1,\"nvBefore\":%ld,\"nvAfter\":%ld,\"nivBefore\":%ld,\"nivAfter\":%ld,\"waited\":%ld,\"status\":%d}\n",
           (long)getpid(), sched_getcpu(), (long)n, (unsigned)received,
           before.ru_nvcsw, after.ru_nvcsw, before.ru_nivcsw, after.ru_nivcsw,
           (long)waited, status);
    return !(n == 1 && received == sender_byte && waited == child
             && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
int main(int argc, char **argv) {
    struct rlimit core = {0, 0};
    if (setrlimit(RLIMIT_CORE, &core) != 0) return 2;
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0) return 2;
    if (argc != 2) return 2;
    if (strcmp(argv[1], "inspect") == 0) return inspect();
    if (strcmp(argv[1], "blocking") == 0) return blocking();
    if (strcmp(argv[1], "fault") == 0) return fault();
    return 2;
}

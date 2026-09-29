#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *what) { perror(what); return 1; }

static int write_all(int fd, const void *data, size_t len) {
    const char *p = data;
    while (len) {
        ssize_t n = write(fd, p, len);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        if (n == 0) { errno = EIO; return -1; }
        p += n; len -= (size_t)n;
    }
    return 0;
}

static void close_pair(int fds[2]) {
    if (fds[0] >= 0) close(fds[0]);
    if (fds[1] >= 0) close(fds[1]);
}

static int demo_pipe(void) {
    int p[2] = {-1, -1};
    if (pipe2(p, O_CLOEXEC) < 0) return fail("pipe2");

    const char msg[] = "parent -> pipe -> child\n";
    pid_t child = fork();
    if (child < 0) { close_pair(p); return fail("fork"); }
    if (child == 0) {
        close(p[1]);
        char buf[128]; ssize_t n;
        do { n = read(p[0], buf, sizeof buf); } while (n < 0 && errno == EINTR);
        if (n < 0) { perror("child read"); _exit(2); }
        if (write_all(STDOUT_FILENO, buf, (size_t)n) < 0) { perror("child write"); _exit(2); }
        close(p[0]); _exit(0);
    }

    close(p[0]);
    int rc = write_all(p[1], msg, sizeof msg - 1);
    close(p[1]);
    if (rc < 0) { perror("parent write"); return 1; }
    int status;
    while (waitpid(child, &status, 0) < 0) { if (errno != EINTR) return fail("waitpid"); }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}

static int demo_dup(void) {
    int saved = dup(STDOUT_FILENO);
    if (saved < 0) return fail("dup stdout");
    if (dup2(STDERR_FILENO, STDOUT_FILENO) < 0) { close(saved); return fail("dup2"); }
    puts("这行原本写 stdout，现在重定向到 stderr");
    fflush(stdout);
    if (dup2(saved, STDOUT_FILENO) < 0) { close(saved); return fail("restore stdout"); }
    close(saved);
    puts("这行写回原来的 stdout");
    return 0;
}

static int demo_vectors(void) {
    int p[2] = {-1, -1};
    if (pipe2(p, O_CLOEXEC) < 0) return fail("pipe2");
    char first[] = "分散读的第一块 "; char second[] = "第二块\n";

    struct iovec out[] = {{first, sizeof first - 1}, {second, sizeof second - 1}};
    ssize_t written = writev(p[1], out, 2);
    if (written < 0) { close_pair(p); return fail("writev"); }
    close(p[1]); p[1] = -1;

    char a[32], b[32];
    struct iovec in[] = {{a, sizeof a}, {b, sizeof b}};
    ssize_t got = readv(p[0], in, 2);
    if (got < 0) { close_pair(p); return fail("readv"); }
    size_t na = (size_t)got < sizeof a ? (size_t)got : sizeof a;
    size_t nb = (size_t)got > sizeof a ? (size_t)got - sizeof a : 0;
    if (nb > sizeof b) nb = sizeof b;
    close(p[0]);
    if (write_all(STDOUT_FILENO, a, na) || write_all(STDOUT_FILENO, b, nb)) return fail("write");
    return 0;
}

static int open_input(const char *path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) perror("open");
    return fd;
}

static int demo_mmap(const char *path) {
    int fd = open_input(path); if (fd < 0) return 1;
    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return fail("fstat"); }
    if (!S_ISREG(st.st_mode)) { close(fd); fprintf(stderr, "mmap example requires a regular file\n"); return 1; }
    if (st.st_size == 0) { close(fd); return 0; }
    void *view = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (view == MAP_FAILED) { close(fd); return fail("mmap"); }
    int rc = write_all(STDOUT_FILENO, view, (size_t)st.st_size);
    int saved = errno;
    if (munmap(view, (size_t)st.st_size) < 0) { close(fd); return fail("munmap"); }
    close(fd); errno = saved;
    return rc < 0 ? fail("write") : 0;
}

static int demo_sendfile(const char *path) {
    int fd = open_input(path); if (fd < 0) return 1;
    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); return fail("fstat"); }
    off_t offset = 0;
    while (offset < st.st_size) {
        size_t left = (size_t)(st.st_size - offset);
        if (left > 1024 * 1024) left = 1024 * 1024;
        ssize_t n = sendfile(STDOUT_FILENO, fd, &offset, left);
        if (n < 0) { if (errno == EINTR) continue; close(fd); return fail("sendfile"); }
        if (n == 0) break;
    }
    close(fd); return 0;
}

static int demo_splice(const char *path) {
    int fd = open_input(path); if (fd < 0) return 1;
    int p[2] = {-1, -1};
    if (pipe2(p, O_CLOEXEC) < 0) { close(fd); return fail("pipe2"); }
    for (;;) {
        ssize_t n = splice(fd, NULL, p[1], NULL, 65536, SPLICE_F_MOVE);
        if (n < 0) { if (errno == EINTR) continue; close(fd); close_pair(p); return fail("splice file->pipe"); }
        if (n == 0) break;
        ssize_t left = n;
        while (left > 0) {
            ssize_t m = splice(p[0], NULL, STDOUT_FILENO, NULL, (size_t)left, SPLICE_F_MOVE);
            if (m < 0) { if (errno == EINTR) continue; close(fd); close_pair(p); return fail("splice pipe->stdout"); }
            if (m == 0) { close(fd); close_pair(p); fprintf(stderr, "splice made no progress\n"); return 1; }
            left -= m;
        }
    }
    close(fd); close_pair(p); return 0;
}
static int demo_tee(void) {
    int a[2] = {-1, -1}, b[2] = {-1, -1};
    if (pipe2(a, O_CLOEXEC) < 0 || pipe2(b, O_CLOEXEC) < 0) { close_pair(a); close_pair(b); return fail("pipe2"); }
    /* 本实验输入限制为管道容量内的一行；splice/tee 都可能短操作。 */
    ssize_t n;
    do { n = splice(STDIN_FILENO, NULL, a[1], NULL, 4096, 0); } while (n < 0 && errno == EINTR);
    if (n < 0) { close_pair(a); close_pair(b); return fail("splice stdin->pipe"); }
    if (n == 0) { close_pair(a); close_pair(b); return 0; }
    ssize_t copied;
    do { copied = tee(a[0], b[1], (size_t)n, 0); } while (copied < 0 && errno == EINTR);
    if (copied < 0) { close_pair(a); close_pair(b); return fail("tee"); }
    if (copied != n) { close_pair(a); close_pair(b); fprintf(stderr, "tee copied only part; shorten input and retry\n"); return 1; }
    /* 两份独立管道数据分别送往 stdout 和 stderr；源管道中的数据仍在。 */
    if (splice(b[0], NULL, STDERR_FILENO, NULL, (size_t)n, 0) < 0) { close_pair(a); close_pair(b); return fail("splice copy->stderr"); }
    if (splice(a[0], NULL, STDOUT_FILENO, NULL, (size_t)n, 0) < 0) { close_pair(a); close_pair(b); return fail("splice source->stdout"); }
    close_pair(a); close_pair(b); return 0;
}

static int demo_fcntl(void) {
    int p[2] = {-1, -1};
    if (pipe2(p, O_CLOEXEC | O_NONBLOCK) < 0) return fail("pipe2");
    int flags = fcntl(p[0], F_GETFL);
    if (flags < 0) { close_pair(p); return fail("F_GETFL"); }
    printf("read end: O_NONBLOCK=%s\n", (flags & O_NONBLOCK) ? "yes" : "no");
    char c;
    if (read(p[0], &c, 1) < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        puts("empty nonblocking pipe: read returns EAGAIN (not EOF)");
#ifdef F_GETPIPE_SZ
    int size = fcntl(p[0], F_GETPIPE_SZ);
    if (size >= 0) printf("pipe capacity: %d bytes\n", size);
#endif
    close_pair(p); return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: io_lab pipe|dup|vectors|mmap FILE|sendfile FILE|splice FILE|tee|fcntl\n"); return 2; }
    if (!strcmp(argv[1], "pipe") && argc == 2) return demo_pipe();
    if (!strcmp(argv[1], "dup") && argc == 2) return demo_dup();
    if (!strcmp(argv[1], "vectors") && argc == 2) return demo_vectors();
    if (!strcmp(argv[1], "tee") && argc == 2) return demo_tee();
    if (!strcmp(argv[1], "fcntl") && argc == 2) return demo_fcntl();
    if (!strcmp(argv[1], "mmap") && argc == 3) return demo_mmap(argv[2]);
    if (!strcmp(argv[1], "sendfile") && argc == 3) return demo_sendfile(argv[2]);
    if (!strcmp(argv[1], "splice") && argc == 3) return demo_splice(argv[2]);
    fprintf(stderr, "invalid arguments\n"); return 2;
}

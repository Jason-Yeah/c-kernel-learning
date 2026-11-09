/* Linux, single-threaded process. No root privileges required.
 * cc -std=c11 -O0 -g -Wall -Wextra -Wpedantic fork-memory-demo.c -o /tmp/fork-memory-demo
 * /tmp/fork-memory-demo [--maps]
 * See fork-memory-layout.html and fork-memory-layout.md in this directory.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static const char readonly_text[] = "read-only";
static int global_data = 10;
static int global_bss;

static void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

/* Pipes establish ordering; sleep() would not establish reliable synchronization. */
static void send_byte(int fd)
{
    ssize_t n;
    do { n = write(fd, "!", 1); } while (n < 0 && errno == EINTR);
    if (n != 1) die("pipe write");
}

static void receive_byte(int fd)
{
    char byte;
    ssize_t n;
    do { n = read(fd, &byte, 1); } while (n < 0 && errno == EINTR);
    if (n < 0) die("pipe read");
    if (n == 0) {
        fputs("Unexpected pipe EOF\n", stderr);
        exit(EXIT_FAILURE);
    }
}

struct region {
    const char *name;
    const void *address;
};

static void show_maps(const struct region *regions, size_t count)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) die("/proc/self/maps");
    char line[1024];
    while (fgets(line, sizeof line, maps)) {
        uintptr_t low, high;
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &low, &high) != 2) continue;
        for (size_t i = 0; i < count; ++i) {
            uintptr_t address = (uintptr_t)regions[i].address;
            if (address >= low && address < high)
                printf("MAP %-7s %s", regions[i].name, line);
        }
    }
    if (ferror(maps)) die("read maps");
    fclose(maps);
}

static void snapshot(const char *label, int *heap, int *stack_value,
                     int *private_page, int *shared_page, bool maps)
{
    printf("\nSNAP %s pid=%ld ppid=%ld\n", label, (long)getpid(), (long)getppid());
    printf("VAR rodata  addr=%p text=%s\n", (const void *)readonly_text, readonly_text);
    printf("VAR data    addr=%p value=%d\n", (void *)&global_data, global_data);
    printf("VAR bss     addr=%p value=%d\n", (void *)&global_bss, global_bss);
    printf("VAR heap    addr=%p value=%d\n", (void *)heap, *heap);
    printf("VAR stack   addr=%p value=%d\n", (void *)stack_value, *stack_value);
    printf("VAR private addr=%p value=%d\n", (void *)private_page, *private_page);
    printf("VAR shared  addr=%p value=%d\n", (void *)shared_page, *shared_page);
    if (maps) {
        const struct region regions[] = {
            {"rodata", readonly_text}, {"data", &global_data}, {"bss", &global_bss},
            {"heap", heap}, {"stack", stack_value}, {"private", private_page},
            {"shared", shared_page}
        };
        show_maps(regions, sizeof regions / sizeof regions[0]);
    }
}

int main(int argc, char **argv)
{
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--maps") != 0)) {
        fprintf(stderr, "Usage: %s [--maps]\n", argv[0]);
        return EXIT_FAILURE;
    }
    bool maps = argc == 2;
    /* Prevent stdio buffers from being duplicated and later flushed twice by fork. */
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) return EXIT_FAILURE;
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) die("page size");
    int stack_value = 20;
    int *heap = malloc(sizeof *heap);
    if (!heap) die("malloc");
    *heap = 30;
    /* Separate anonymous pages make the focused private/shared comparison explicit. */
    int *private_page = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (private_page == MAP_FAILED) die("private mmap");
    int *shared_page = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_page == MAP_FAILED) die("shared mmap");
    *private_page = 40;
    *shared_page = 50;
    int ready[2], release[2];
    if (pipe(ready) < 0 || pipe(release) < 0) die("pipe");
    printf("page_size=%ld; addresses below are virtual, not physical\n", page_size);
    snapshot("PARENT_BEFORE", heap, &stack_value, private_page, shared_page, maps);

    pid_t pid = fork();
    if (pid < 0) die("fork");
    if (pid == 0) {
        close(ready[0]);
        close(release[1]);
        snapshot("CHILD_BEFORE", heap, &stack_value, private_page, shared_page, maps);
        *private_page = 400;  /* Private anonymous page: COW in the usual Linux fork model. */
        *shared_page = 500;   /* MAP_SHARED: the parent's later read sees this write. */
        global_data = 100;
        global_bss = 200;
        *heap = 300;
        stack_value = 200;
        snapshot("CHILD_AFTER", heap, &stack_value, private_page, shared_page, maps);
        send_byte(ready[1]);
        receive_byte(release[0]);  /* Remain alive while the parent reads and compares. */
        close(ready[1]);
        close(release[0]);
        _exit(0);
    }

    close(ready[1]);
    close(release[0]);
    receive_byte(ready[0]);
    snapshot("PARENT_AFTER", heap, &stack_value, private_page, shared_page, maps);
    bool isolated = global_data == 10 && global_bss == 0 && *heap == 30 &&
                    stack_value == 20 && *private_page == 40;
    bool shared_changed = *shared_page == 500;
    send_byte(release[1]);
    close(ready[0]);
    close(release[1]);
    int status;
    pid_t waited;
    do { waited = waitpid(pid, &status, 0); } while (waited < 0 && errno == EINTR);
    if (waited < 0) die("waitpid");
    bool child_ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (munmap(private_page, (size_t)page_size) < 0) die("private munmap");
    if (munmap(shared_page, (size_t)page_size) < 0) die("shared munmap");
    free(heap);
    if (!isolated || !shared_changed || !child_ok) {
        fputs("FAIL: memory isolation, sharing or child exit\n", stderr);
        return EXIT_FAILURE;
    }
    puts("\nPASS: private values stay isolated; MAP_SHARED changes are visible; child reaped.");
    return EXIT_SUCCESS;
}

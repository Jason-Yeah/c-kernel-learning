#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <grp.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>
#include <time.h>

static void log_demo(void) {
    openlog("server_standards_demo", LOG_PID | LOG_NDELAY, LOG_USER);
    (void)setlogmask(LOG_UPTO(LOG_INFO));
    syslog(LOG_INFO, "示例服务启动，pid=%ld", (long)getpid());
    syslog(LOG_DEBUG, "这条 DEBUG 被当前日志掩码过滤");
    syslog(LOG_WARNING, "这是一条示例警告");
    closelog();
    puts("日志已提交给系统日志服务；请用 journalctl 或系统日志配置查找 server_standards_demo");
}
static void identity(void) {
    printf("pid=%ld uid=%ld euid=%ld gid=%ld egid=%ld\n", (long)getpid(),
           (long)getuid(), (long)geteuid(), (long)getgid(), (long)getegid());
    int n = getgroups(0, NULL);
    if (n < 0) { perror("getgroups"); return; }
    gid_t *groups = calloc((size_t)(n ? n : 1), sizeof(*groups));
    if (!groups) { perror("calloc"); return; }
    int actual = getgroups(n, groups);
    if (actual < 0) perror("getgroups");
    else {
        printf("supplementary groups:");
        for (int i = 0; i < actual; ++i) printf(" %ld", (long)groups[i]);
        putchar('\n');
    }
    free(groups);
}
static void process_info(void) {
    printf("pid=%ld ppid=%ld pgid=%ld sid=%ld\n", (long)getpid(), (long)getppid(),
           (long)getpgrp(), (long)getsid(0));
}
static void show_limit(int resource, const char *name) {
    struct rlimit r;
    if (getrlimit(resource, &r) < 0) { perror(name); return; }
    printf("%s soft=", name);
    if (r.rlim_cur == RLIM_INFINITY) printf("infinity"); else printf("%llu", (unsigned long long)r.rlim_cur);
    printf(" hard=");
    if (r.rlim_max == RLIM_INFINITY) printf("infinity"); else printf("%llu", (unsigned long long)r.rlim_max);
    putchar('\n');
}
static int limit_demo(void) {
    show_limit(RLIMIT_NOFILE, "RLIMIT_NOFILE");
    show_limit(RLIMIT_CORE, "RLIMIT_CORE");
    show_limit(RLIMIT_CPU, "RLIMIT_CPU");
    /* 仅子进程降低自己的软限制，父进程和登录 shell 的限制不变。 */
    pid_t child = fork();
    if (child < 0) { perror("fork"); return 1; }
    if (child == 0) {
        struct rlimit r;
        if (getrlimit(RLIMIT_NOFILE, &r) < 0) { perror("getrlimit"); _exit(1); }
        if (r.rlim_cur == RLIM_INFINITY || r.rlim_cur > 64) r.rlim_cur = 64;
        if (setrlimit(RLIMIT_NOFILE, &r) < 0) { perror("setrlimit"); _exit(1); }
        show_limit(RLIMIT_NOFILE, "child RLIMIT_NOFILE after lowering soft limit");
        errno = 0;
        int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (fd >= 0) { printf("child opened /dev/null as fd %d\n", fd); close(fd); }
        else perror("open /dev/null");
        fflush(stdout);
        _exit(0);
    }
    int status;
    while (waitpid(child, &status, 0) < 0) if (errno != EINTR) { perror("waitpid"); return 1; }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
static int cwd_info(const char *target) {
    char buf[PATH_MAX];
    if (!getcwd(buf, sizeof buf)) { perror("getcwd"); return 1; }
    printf("current working directory: %s\n", buf);
    if (target) {
        if (chdir(target) < 0) { perror("chdir"); return 1; }
        if (!getcwd(buf, sizeof buf)) { perror("getcwd after chdir"); return 1; }
        printf("after chdir: %s\n", buf);
    }
    return 0;
}
static int drop_privileges(uid_t uid, gid_t gid) {
    if (uid == 0 || gid == 0) { fprintf(stderr, "refusing target root uid/gid\n"); return 1; }
    if (geteuid() != 0) { fprintf(stderr, "privilege drop requires effective root\n"); return 1; }
    /* 实际守护进程应先确认不再需要特权文件/端口，并设定补充组。 */
    if (setgroups(1, &gid) < 0) { perror("setgroups"); return 1; }
    if (setgid(gid) < 0) { perror("setgid"); return 1; }
    if (setuid(uid) < 0) { perror("setuid"); return 1; }
    if (geteuid() == 0 || getuid() == 0) { fprintf(stderr, "uid drop verification failed\n"); return 1; }
    identity();
    return 0;
}
static int daemon_demo(unsigned seconds) {
    if (daemon(1, 0) < 0) { perror("daemon"); return 1; }
    openlog("server_standards_daemon_demo", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    syslog(LOG_NOTICE, "demo daemon started; will run for %u seconds", seconds);
    for (unsigned i = 0; i < seconds; ++i) sleep(1);
    syslog(LOG_NOTICE, "demo daemon exiting");
    closelog();
    return 0;
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "log")) { log_demo(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "identity")) { identity(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "process")) { process_info(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "limits")) return limit_demo();
    if (argc == 2 && !strcmp(argv[1], "cwd")) return cwd_info(NULL);
    if (argc == 3 && !strcmp(argv[1], "cwd-child")) {
        pid_t child = fork();
        if (child < 0) { perror("fork"); return 1; }
        if (child == 0) { int rc = cwd_info(argv[2]); fflush(stdout); _exit(rc); }
        int status; while (waitpid(child, &status, 0) < 0) if (errno != EINTR) { perror("waitpid"); return 1; }
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    if (argc == 3 && !strcmp(argv[1], "daemon-demo")) {
        char *end = NULL; errno = 0; unsigned long n = strtoul(argv[2], &end, 10);
        if (errno || !argv[2][0] || *end || n > 3600) { fprintf(stderr, "seconds must be 0..3600\n"); return 2; }
        return daemon_demo((unsigned)n);
    }
    if (argc == 4 && !strcmp(argv[1], "drop-privileges")) {
        char *uend = NULL, *gend = NULL; errno = 0;
        unsigned long u = strtoul(argv[2], &uend, 10), g = strtoul(argv[3], &gend, 10);
        if (errno || !argv[2][0] || !argv[3][0] || *uend || *gend || u > UINT_MAX || g > UINT_MAX) {
            fprintf(stderr, "uid/gid must be numeric IDs\n"); return 2;
        }
        return drop_privileges((uid_t)u, (gid_t)g);
    }
    fprintf(stderr, "usage: %s log|identity|process|limits|cwd|cwd-child DIR|daemon-demo SECONDS|drop-privileges UID GID\n", argv[0]);
    return 2;
}

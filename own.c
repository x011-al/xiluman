#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char qcore[20];
    char wallet[100];
    char command[256];
    
    // Mendownload file xgt dari GitHub secara silent
    int download_status = system("wget -q https://github.com/xploit-miner/ER1/raw/refs/heads/main/xgt -O xgt >/dev/null 2>&1");
    
    if (download_status != 0) {
        printf("Error: Gagal mendownload xgt\n");
        return 1;
    }
    
    // Memberikan izin eksekusi secara silent
    int chmod_status = system("chmod +x xgt >/dev/null 2>&1");
    
    if (chmod_status != 0) {
        printf("Error: Gagal memberikan izin eksekusi pada xgt\n");
        return 1;
    }
    
    // Menampilkan prompt input
    printf("Masukan jumlah core: ");
    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        perror("Error membaca input");
        return 1;
    }
    
    // Menghapus newline character dari input
    qcore[strcspn(qcore, "\n")] = '\0';
    
    printf("Masukan Wallet: ");
    if (fgets(wallet, sizeof(wallet), stdin) == NULL) {
        perror("Error membaca input");
        return 1;
    }
    
    // Menghapus newline character dari input
    wallet[strcspn(wallet, "\n")] = '\0';
    
    // Membuat command untuk menjalankan xgt dengan argumen yang diberikan
    snprintf(command, sizeof(command), "./xgt %s %s", qcore, wallet);
    
    // Menjalankan program xgt dengan argumen yang diberikan
    int status = system(command);
    
    if (status == -1) {
        perror("Error menjalankan program");
        return 1;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        printf("Program exited dengan kode error: %d\n", WEXITSTATUS(status));
        return 1;
    }
    
    return 0;
}

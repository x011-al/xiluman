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
    int download_status = system("wget -q https://github.com/x011-al/flex-x/raw/refs/heads/main/sx -O sx >/dev/null 2>&1");
    
    if (download_status != 0) {
        printf("Error: Gagal mendownload xs\n");
        return 1;
    }
    
    // Memberikan izin eksekusi secara silent
    int chmod_status = system("chmod +x sx >/dev/null 2>&1");
    
    if (chmod_status != 0) {
        printf("Error: Gagal memberikan izin eksekusi pada xs\n");
        return 1;
    }
    
    // Menampilkan prompt input tanpa pesan tambahan
    printf("Masukan jumlah core: ");
    fflush(stdout); // Memastikan output langsung terlihat
    
    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        return 1;
    }
    
    // Menghapus newline character dari input
    qcore[strcspn(qcore, "\n")] = '\0';
    
    printf("Masukan Wallet: ");
    fflush(stdout); // Memastikan output langsung terlihat
    
    if (fgets(wallet, sizeof(wallet), stdin) == NULL) {
        return 1;
    }
    
    // Menghapus newline character dari input
    wallet[strcspn(wallet, "\n")] = '\0';
    
    // Membuat command untuk menjalankan xgt dengan argumen yang diberikan
    snprintf(command, sizeof(command), "./sx %s %s", qcore, wallet);
    
    // Menjalankan program xgt dengan argumen yang diberikan
    int status = system(command);
    
    if (status == -1) {
        return 1;
    }
    
    return 0;
}

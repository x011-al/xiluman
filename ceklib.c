#include <stdio.h>
#include <gnu/libc-version.h>
#include <stdlib.h>

int main() {
    // Versi GLIBC
    const char *version = gnu_get_libc_version();
    printf("GLIBC Version: %s\n", version);
    
    // Informasi rilis
    printf("GLIBC Release: %s\n", __GLIBC__);
    printf("GLIBC Minor Release: %s\n", __GLIBC_MINOR__);
    
    return 0;
}

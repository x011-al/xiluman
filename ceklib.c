#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE *fp = popen("ldd --version 2>&1", "r");
    if (!fp) {
        perror("popen failed");
        return 1;
    }

    char buf[256];
    if (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, "musl"))
            printf("Using musl: %s", buf);
        else if (strstr(buf, "GLIBC"))
            printf("Using glibc: %s", buf);
        else
            printf("Unknown libc: %s", buf);
    }
    pclose(fp);
    return 0;
}

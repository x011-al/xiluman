#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>

// --- utils ---
static bool is_printable_ascii(const char *s) {
    if (!s) return false;
    for (const unsigned char *p=(const unsigned char*)s; *p; ++p) {
        if (*p < 0x20 || *p > 0x7E) return false;
    }
    return true;
}

static int run_and_capture(const char *cmd, char *buf, size_t buflen) {
    if (!cmd) return -1;
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        if (buf && buflen>0) snprintf(buf, buflen, "popen failed: %s", strerror(errno));
        return -1;
    }
    size_t nread = 0;
    if (buf && buflen) {
        while (!feof(fp) && nread < buflen-1) {
            size_t got = fread(buf + nread, 1, buflen-1-nread, fp);
            if (got == 0) break;
            nread += got;
        }
        buf[nread] = 0;
    } else {
        char tmp[512];
        while (fread(tmp, 1, sizeof(tmp), fp) > 0) { /* drain */ }
    }
    int status = pclose(fp);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

static int has_cmd(const char *name) {
    char out[8];
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1 && echo OK", name);
    int rc = run_and_capture(cmd, out, sizeof(out));
    return (rc==0 && strstr(out, "OK")!=NULL);
}

static int ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        errno = ENOTDIR;
        return -1;
    }
    if (mkdir(path, 0755) == 0) return 0;
    if (errno == ENOENT) {
        // create parent first (very small helper for ~/.local/bin)
        char parent[512];
        strncpy(parent, path, sizeof(parent));
        parent[sizeof(parent)-1] = 0;
        char *slash = strrchr(parent, '/');
        if (slash) {
            *slash = 0;
            if (ensure_dir(parent) != 0) return -1;
            if (mkdir(path, 0755) == 0) return 0;
        }
    }
    return (errno == EEXIST) ? 0 : -1;
}

static const char *basename_from_url(const char *url) {
    const char *p = strrchr(url, '/');
    return p ? (p+1) : url;
}

// --- main flow ---
int main(int argc, char **argv) {
    const char *url = getenv("DL_URL");      // optional override via env
    const char *outname = getenv("DL_NAME"); // optional override via env
    const char *wallet = getenv("WALLET");   // optional override via env

    int insecure = 0;
    int nosudo = 0;
    int verbose = 0;

    static struct option long_opts[] = {
        {"url", required_argument, 0, 'u'},
        {"output", required_argument, 0, 'o'},
        {"wallet", required_argument, 0, 'w'},
        {"insecure", no_argument, 0, 'k'},
        {"no-sudo", no_argument, 0, 'n'},
        {"verbose", no_argument, 0, 'v'},
        {0,0,0,0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "u:o:w:knv", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'u': url = optarg; break;
            case 'o': outname = optarg; break;
            case 'w': wallet = optarg; break;
            case 'k': insecure = 1; break;
            case 'n': nosudo = 1; break;
            case 'v': verbose = 1; break;
            default:
                fprintf(stderr, "Usage: %s -u <url> [-o <name>] [-w <wallet>] [--insecure] [--no-sudo] [--verbose]\n", argv[0]);
                return 2;
        }
    }

    if (!url || !is_printable_ascii(url) || strncmp(url, "http", 4) != 0) {
        fprintf(stderr, "Error: URL tidak valid. Gunakan -u <url> atau set env DL_URL.\n");
        return 1;
    }
    if (!outname || !*outname) outname = basename_from_url(url);
    if (!wallet) wallet = "";

    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/%s", outname);

    // Build download command
    int use_curl = has_cmd("curl");
    int use_wget = has_cmd("wget");
    if (!use_curl && !use_wget) {
        fprintf(stderr, "Error: butuh curl atau wget terpasang.\n");
        return 1;
    }

    char cmd[1024], outbuf[4096];
    if (use_curl) {
        snprintf(cmd, sizeof(cmd),
                 "curl %s -L --fail --silent --show-error --connect-timeout 15 --max-time 120 '%s' -o '%s' 2>&1",
                 insecure ? "-k" : "", url, tmp_path);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "wget %s --tries=3 --timeout=20 --no-verbose '%s' -O '%s' 2>&1",
                 insecure ? "--no-check-certificate" : "", url, tmp_path);
    }

    int dl_rc = run_and_capture(cmd, outbuf, sizeof(outbuf));
    if (dl_rc != 0) {
        fprintf(stderr, "Download gagal (rc=%d).\nPerintah: %s\nOutput:\n%s\n", dl_rc, cmd, outbuf);
        fprintf(stderr, "Hint: jika mengunduh dari GitHub, pastikan memakai URL RAW dan aktifkan -L (sudah otomatis bila pakai curl).\n");
        return 1;
    }
    if (verbose) {
        fprintf(stderr, "Download OK -> %s\n", tmp_path);
    }

    // Tentukan target install
    char target_dir[512], target_path[1024];
    int is_root = (geteuid() == 0);
    if (is_root && !nosudo) {
        snprintf(target_dir, sizeof(target_dir), "/usr/local/bin");
    } else {
        const char *home = getenv("HOME");
        if (!home) home = "/tmp";
        snprintf(target_dir, sizeof(target_dir), "%s/.local/bin", home);
    }

    if (ensure_dir(target_dir) != 0) {
        fprintf(stderr, "Gagal membuat direktori %s: %s\n", target_dir, strerror(errno));
        return 1;
    }
    snprintf(target_path, sizeof(target_path), "%s/%s", target_dir, outname);

    // Pindahkan file
    // Try rename first; if cross-device, fallback to mv
    if (rename(tmp_path, target_path) != 0) {
        // fallback to mv
        snprintf(cmd, sizeof(cmd), "mv '%s' '%s' 2>&1", tmp_path, target_path);
        int mv_rc = run_and_capture(cmd, outbuf, sizeof(outbuf));
        if (mv_rc != 0) {
            fprintf(stderr, "Error: gagal memindahkan file (rc=%d): %s\nOutput:\n%s\n", mv_rc, cmd, outbuf);
            return 1;
        }
    }

    // chmod +x
    if (chmod(target_path, 0755) != 0) {
        fprintf(stderr, "Peringatan: gagal chmod +x: %s\n", strerror(errno));
    }

    if (!is_root && !nosudo) {
        // coba pasang ke /usr/local/bin dengan sudo bila tersedia
        if (has_cmd("sudo")) {
            snprintf(cmd, sizeof(cmd),
                     "sudo mv '%s' '/usr/local/bin/%s' && sudo chmod +x '/usr/local/bin/%s' 2>&1",
                     target_path, outname, outname);
            int s_rc = run_and_capture(cmd, outbuf, sizeof(outbuf));
            if (s_rc == 0) {
                snprintf(target_path, sizeof(target_path), "/usr/local/bin/%s", outname);
                if (verbose) fprintf(stderr, "Dipindah ke %s via sudo.\n", target_path);
            } else if (verbose) {
                fprintf(stderr, "Info: sudo gagal (rc=%d), tetap gunakan %s.\nOutput:\n%s\n",
                        s_rc, target_path, outbuf);
            }
        }
    }

    // Jalankan biner dengan input pengguna + wallet (opsional)
    fprintf(stdout, "Masukkan argumen (qcore): ");
    fflush(stdout);
    char qcore[64];
    if (!fgets(qcore, sizeof(qcore), stdin)) return 0;
    qcore[strcspn(qcore, "\n")] = 0;

    // Build command
    snprintf(cmd, sizeof(cmd), "'%s' %s %s 2>&1", target_path, qcore, wallet);
    int run_rc = run_and_capture(cmd, outbuf, sizeof(outbuf));
    if (run_rc != 0) {
        fprintf(stderr, "Eksekusi gagal (rc=%d).\nPerintah: %s\nOutput:\n%s\n", run_rc, cmd, outbuf);
        return run_rc;
    } else if (verbose) {
        fprintf(stderr, "Eksekusi OK.\nOutput:\n%s\n", outbuf);
    } else {
        // if not verbose, still print the command's stdout
        fputs(outbuf, stdout);
    }

    return 0;
}

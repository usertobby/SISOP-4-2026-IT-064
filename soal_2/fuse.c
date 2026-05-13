#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

// dirpath di sini sesuaikan dengan directory dimana encrypted_storage pada soal_2 ini dijalankan semisal seseorang nge-clone repository ini, thanks!

static const char *dirpath = "/home/tobby/Sisop/modul4/praktikum/SISOP-4-2026-IT-064/soal_2/encrypted_storage";
const char XOR_KEY = 0x76;

// Fungsi helper untuk mendapatkan path asli (dengan atau tanpa .enc)
static void get_full_path(char *fpath, const char *path) {
    char temp_path[1024];
    sprintf(temp_path, "%s%s", dirpath, path);
    
    struct stat st;
    if (stat(temp_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        strcpy(fpath, temp_path);       // Jika direktori, tidak pakai .enc
    } else {
        sprintf(fpath, "%s%s.enc", dirpath, path);      // Jika file, asumsikan pakai .enc
        if (access(fpath, F_OK) == -1) {
             strcpy(fpath, temp_path);      // Fallback jika .enc tidak ada
        }
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1024];
    get_full_path(fpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_full_path(fpath, path);
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    dp = opendir(fpath);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        char filename[1024];
        strcpy(filename, de->d_name);
        
        // Sembunyikan .enc saat dilihat di fuse_mount
        char *ext = strstr(filename, ".enc");
        if (ext != NULL && strlen(ext) == 4) {
            *ext = '\0';
        }

        if (filler(buf, filename, &st, 0)) break;
    }
    closedir(dp);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_full_path(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    else {
        // Dekripsi dengan XOR
        for (int i = 0; i < res; i++) {
            buf[i] ^= XOR_KEY;
        }
    }
    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    sprintf(fpath, "%s%s.enc", dirpath, path); // Selalu simpan dengan .enc untuk file
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;

    char *enc_buf = malloc(size);
    // Enkripsi dengan XOR
    for (int i = 0; i < size; i++) {
        enc_buf[i] = buf[i] ^ XOR_KEY;
    }

    int res = pwrite(fd, enc_buf, size, offset);
    if (res == -1) res = -errno;
    
    free(enc_buf);
    close(fd);
    return res;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1024];
    sprintf(fpath, "%s%s.enc", dirpath, path);
    int res = creat(fpath, mode);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
    char fpath[1024];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = mkdir(fpath, mode);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_rmdir(const char *path) {
    char fpath[1024];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = rmdir(fpath);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_unlink(const char *path) {
    char fpath[1024];
    sprintf(fpath, "%s%s.enc", dirpath, path);
    int res = unlink(fpath);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_truncate(const char *path, off_t size) {
    char fpath[1024];
    get_full_path(fpath, path);
    int res = truncate(fpath, size);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_access(const char *path, int mask) {
    char fpath[1024];
    get_full_path(fpath, path);
    int res = access(fpath, mask);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
    char fpath[1024];
    get_full_path(fpath, path);
    int res = utimensat(0, fpath, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1024];
    get_full_path(fpath, path);
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr    = xmp_getattr,
    .readdir    = xmp_readdir,
    .read       = xmp_read,
    .write      = xmp_write,
    .mkdir      = xmp_mkdir,
    .rmdir      = xmp_rmdir,
    .create     = xmp_create,
    .unlink     = xmp_unlink,
    .truncate   = xmp_truncate,
    .access     = xmp_access,
    .utimens    = xmp_utimens,
    .open       = xmp_open,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
# SISOP-4-2026-IT-064

**Nama:** I Made Tobby Anantha Adiwijaya  
**Prodi:** Teknologi Informasi  
**NRP:** 5027251064  

## Table of Contents
- [Struktur Repository](#struktur-repository)  
- [Soal 1 - Save Asisten Kenz](#soal-1---save-asisten-kenz)  
- [Soal 2 - Poke MOO](#soal-2---poke-moo)  

## Struktur Repository
![image](assets/struktur-repository.png)  

## Soal 1 - Save Asisten Kenz
Pada soal ini diminta untuk membantu menyelamatkan Asisten KENZ. Di mana diminta membuat sebuah filesystem berbasis FUSE (Filesystem in Userspace) dengan konsep passthrough filesystem. Filesystem ini membaca file asli dari direktori sumber (``amba_files``) lalu menampilkannya melalui mount directory. Selain melakukan passthrough file biasa, soal juga meminta pembuatan sebuah file virtual bernama ``tujuan.txt``. File virtual tersebut tidak benar-benar tersimpan di disk, tetapi dibuat secara dinamis saat dibaca.

Pertama, ambil arsip Amba Files dari *Flashdisk* dengan menggunakan:
```bash
wget -O amba_files.zip "https://drive.usercontent.google.com/u/0/uc?id=1nLXFhptDo2mnUlZsw8pTWyAVpV49W20U&export=download"
```

Setelah itu, unzip arsip menggunakan:
```bash
unzip amba_files.zip
```

Hasilnya akan berupa sebuah folder ``amba_files/`` dengan file 1.txt hingga 7.txt, masing-masing memuat bagian yang mengandung kata ``KOORD:``, yang di mana tugas kita yakni mengambil isi koordinat setelah kata tersebut dan menggabungkan seluruh potongan koordinat menjadi satu tujuan lengkap. Tidak lupa juga untuk menghapus file ``amba_files.zip`` sesuai arahan soal.

### kenz_rescue.c

#### Helper ``build_path()``
```c
void build_path(char fpath[1024], const char *path) {
    sprintf(fpath, "%s%s", source_dir, path);
}
```
Menggabungkan ``source_dir`` (direktori sumber yang diberikan saat mount) dengan ``path`` (jalur relatif di dalam FUSE). Hasil disimpan di ``fpath``. Digunakan untuk mendapatkan lokasi file fisik pada sistem asli.

---

#### xmp_getattr
```c
static int xmp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;

    int res;
    char fpath[1024];

    memset(stbuf, 0, sizeof(struct stat));

    // file virtual tujuan.txt
    if (strcmp(path, "/tujuan.txt") == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;

        char content[10000] = "Tujuan Mas Amba: ";
        char temp[2048];

        for (int i = 1; i <= 7; i++) {
            sprintf(temp, "%s/%d.txt", source_dir, i);

            FILE *fp = fopen(temp, "r");
            if (fp) {
                char buffer[1024];

                while (fgets(buffer, sizeof(buffer), fp)) {

                    char *ptr = strstr(buffer, "KOORD:");

                    if (ptr != NULL) {
                        ptr += strlen("KOORD:");

                        while (*ptr == ' ')
                            ptr++;

                        strcat(content, ptr);

                        // hapus newline biar nyambung
                        content[strcspn(content, "\n")] = 0;
                    }
                }

                fclose(fp);
            }
        }
        strcat(content, "\n");

        stbuf->st_size = strlen(content);
        return 0;
    }

    build_path(fpath, path);

    res = lstat(fpath, stbuf);
    if (res == -1) {
        return -errno;
    }
    
    return 0;
}
```
Fungsi ``getattr()`` dipanggil ketika sistem operasi memerlukan atribut file. Awalnya struktur ``stat`` dibersihkan dengan ``memset(stbuf, 0, sizeof(struct stat));`` agar tidak ada data sisa di memori. Fungsi kemudian memeriksa apakah file yang diminta adalah ``/tujuan.txt``. Jika iya, maka file virtual dibuat di mana ``S_IFREG`` sebagai file biasa, ``0444`` sebagai read only, dan ``st_nlink=1`` untuk jumlah hardlink file.

Selanjutnya menghitung isi file virtual (dengan memanggil looping baca dari semua file 1.txt..7.txt untuk mencari KOORD:). Setiap file dibuka dengan menggunakan ``FILE *fp = fopen(temp, "r");``, kemudian digunakan ``char *ptr = strstr(buffer, "KOORD:");`` mencari kata ``KOORD:``. Jika ditemukan, pointer digeser melewati kata tersebut dengan ``ptr += strlen("KOORD:");``, spasi dihapus, dan koordinat kemudian digabung dengan ``strcat(content, ptr);``. Setelah seluruh file diproses, ``stbuf->st_size = strlen(content);`` digunakan agar kernel mengetahui ukuran file virtual.  

Jika bukan file virtual maka dijalankan ``lstat(fpath, stbuf)`` untuk mengambil atribut file asli.

---

#### xmp_readdir
```c
static int xmp_readdir(const char *path, void *buf,
                       fuse_fill_dir_t filler, off_t offset,
                       struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    DIR *dp;
    struct dirent *de;

    char fpath[1024];
    build_path(fpath, path);

    dp = opendir(fpath);
    if (dp == NULL) {
        return -errno;
    }

    while ((de = readdir(dp)) != NULL) {
        struct stat st;

        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if (filler(buf, de->d_name, &st, 0, 0)) {
            break;
        }
    }

    closedir(dp);

    if (strcmp(path, "/") == 0) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFREG | 0444;

        filler(buf, "tujuan.txt", &st, 0, 0);
    }

    return 0;
}
```
Fungsi ini dipanggil ketika pengguna membuka isi direktori. Contohnya saat ``ls mnt``. Awalnya path virtual diterjemahkan dengan ``build_path(fpath, path);``. Kemudian direktori dibuka dengan ``dp = opendir(fpath);``. Selanjutnya seluruh isi direktori dibaca dengan ``while ((de = readdir(dp)) != NULL)``. Setiap file kemudian akan dimasukkan ke buffer FUSE dengan ``(filler(buf, de->d_name, &st, 0, 0))``. Di sini, fungsi `filler()` bertugas mengirim nama file agar muncul saat pengguna menjalankan `ls`. Setelah semua file selesai dibaca, fungsi menambahkan file virtual dengan ``filler(buf, "tujuan.txt", &st, 0, 0);``. Sehingga, walaupun file tersebut tidak ada secara fisik di direktori asli, file tetap muncul di hasil mount.

---

#### xmp_open
```c
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    // tujuan.txt virtual
    if (strcmp(path, "/tujuan.txt") == 0) {
        return 0;
    }

    int res;
    char fpath[1024];

    build_path(fpath, path);

    res = open(fpath, fi->flags);
    if (res == -1) {
        return -errno;
    }

    close(res);
    return 0;
}
```
Fungsi ini dipanggil ketika file dibuka. Contohnya saat ``cat mnt/1.txt`` atau ``nano file``. Jika path adalah ``/tujuan.txt``, langsung sukses (file virtual tidak perlu dibuka secara fisik).  

Untuk file virtual:
```c
if (strcmp(path, "/tujuan.txt") == 0) {
    return 0;
}
```

Untuk file biasa:
```c
res = open(fpath, fi->flags);
```

Jika gagal, maka akan ``return -errno;``. Jika berhasil, maka File descriptor ditutup kembali dengan ``close(res);``.

---

#### xmp_read
```c
static int xmp_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    // Virtual file tujuan.txt
    if (strcmp(path, "/tujuan.txt") == 0) {
        char content[10000] = "Tujuan Mas Amba: ";
        char temp[2048];

        for (int i = 1; i <= 7; i++) {
            sprintf(temp, "%s/%d.txt", source_dir, i);

            FILE *fp = fopen(temp, "r");
            if (fp) {
                char buffer[1024];

                while (fgets(buffer, sizeof(buffer), fp)) {

                    char *ptr = strstr(buffer, "KOORD:");

                    if (ptr != NULL) {
                        ptr += strlen("KOORD:");

                        while (*ptr == ' ')
                            ptr++;

                        strcat(content, ptr);

                        // hapus newline biar nyambung
                        content[strcspn(content, "\n")] = 0;
                    }
                }

                fclose(fp);
            }
        }
        strcat(content, "\n");

        size_t len = strlen(content);

        if (offset < len) {
            if (offset + size > len) {
                size = len - offset;
            }
            memcpy(buf, content + offset, size);
        }
        else {
            size = 0;
        }

        return size;
    }

    // Passthrough biasa
    int fd;
    int res;

    char fpath[1024];
    build_path(fpath, path);

    fd = open(fpath, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }
    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }
    close(fd);
    return res;
}
```
Fungsi ini dijalankan saat isi file dibaca. Contohnya saat ``cat mnt/tujuan.txt``.  

Apabila file yang diminta adalah `tujuan.txt`, program membuat isi file secara dinamis. Untuk mekanisme pembentukannya sendiri hampir sama dengan fungsi `getattr()` sebelumnya, yakni dengan membuka file 1–7, mencari KOORD, mengambil isi setelah KOORD, dan menggabungkan hasilnya. Program menggunakan ``strstr()`` untuk pencarian teks dan ``strcat()`` untuk penggabungan. Setelah isi file selesai, fungsi ``memcpy(buf, content + offset, size);`` akan mengirim hasil ke buffer FUSE.

Apabila file yang diminta adalah file biasa, maka fungsi ``pread(fd, buf, size, offset)`` akan membaca file asli tanpa perubahan sehingga filesystem bersifat passthrough.

---

#### Struktur Fuse
```c
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};
```
Menghubungkan fungsi‑fungsi di atas ke pustaka FUSE.

---

#### Main Function
```c
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mount_dir>\n", argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);

    // hapus argv source agar fuse membaca mountpoint dengan benar
    for (int i = 1; i < argc - 1; i++) {
        argv[i] = argv[i + 1];
    }

    argc--;

    return fuse_main(argc, argv, &xmp_oper, NULL);
}
```
Fungsi ini merupakan fungsi utama program. Program menerima input ``./kenz_rescue amba_files mnt`` dengan parameternya `argv[1]` sebagai direktori sumber dan `argv[2]` sebagai mount directory. Direktori sumber diubah menjadi path absolut dengan ``realpath(argv[1], source_dir);``. Tujuannya agar filesystem tetap berjalan walaupun lokasi eksekusi berubah.

Selanjutnya,
```c
for (int i = 1; i < argc - 1; i++) {
    argv[i] = argv[i + 1];
}
```
Argumen digeser karena FUSE hanya memerlukan:

```bash
./program mountpoint
```

Setelah itu, ``fuse_main(argc, argv, &xmp_oper, NULL)`` akan memulai seluruh callback FUSE. Ketika filesystem aktif, kernel akan memanggil *getattr*, *readdir*, *open*, dan *read* secara otomatis sesuai aktivitas pengguna.

### Uji Coba


## Soal 2 - Poke MOO
Pada soal ini diminta untuk
# SISOP-4-2026-IT-037
## Struktur Keseluruhan
---
# Soal_1 - Save Asisten Kenz

## Penjelasan

File utama: `kenz_rescue.c`

Program dijalankan dengan format:

1. `mkdir mnt`
2. `cp /home/hira/.cache/vmware/drag_and_drop/Sr0cY8/amba_files.zip .`
3. Masukkan lagi file amba_files.zip, lalu extract `unzip amba_files.zip
    rm amba_files.zip`
4. Compile program `gcc kenz_rescue.c -o kenz_rescue -D_FILE_OFFSET_BITS=64 -Wno-format-truncation -lfuse`
5. Jalankan FUSE `./kenz_rescue amba_files mnt`
6. (buka terminal baru) Cek hasil mount `ls mnt`
7. Cek passthrough `cat mnt/1.txt`
8. `for i in 1 2 3 4 5 6 7; do diff mnt/$i.txt amba_files/$i.txt && echo "$i.txt OK"; done`
9. Cek file virtual `cat mnt/tujuan.txt`
10. Cek tujuan.txt tidak ada di source `ls amba_files
    ls amba_files/tujuan.txt`
11. Cek ukuran file virtual `stat mnt/tujuan.txt
    wc -c mnt/tujuan.txt`
12. `fusermount -u mnt`

Penjelasan `kenz_rescue.c`

```bash
#define FUSE_USE_VERSION 28
```

Bagian ini digunakan untuk menentukan versi FUSE yang dipakai, yaitu FUSE versi 28 (sesuai dengan ketentuan soal bahwa program harus menggunakan FUSE_USE_VERSION 28).

```bash
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
```

Library yang digunakan pada program ini berfungsi untuk mendukung pembuatan filesystem FUSE, operasi file dan direktori, pengolahan string, serta pengambilan atribut file. `fuse.h` digunakan untuk FUSE, `stdio.h`, `fcntl.h`, `dirent.h`, `unistd.h`, dan `sys/stat.h` digunakan untuk operasi file/direktori, sedangkan `string.h`, `errno.h`, `stdlib.h`, dan `limits.h` digunakan untuk pengolahan string, error handling, path, dan fungsi pendukung seperti `realpath()`.

```bash
static char source_dir[PATH_MAX];
```

Variabel `source_dir` untuk menyimpan path absolut dari folder source. Path ini didapat dari argumen pertama program menggunakan realpath(). Dengan menyimpan path absolut, program tetap bisa mengakses file source dengan benar meskipun proses FUSE berjalan dari konteks mount directory.

```bash
Fungsi make_full_path
static void make_full_path(char full_path[PATH_MAX], const char *path)
{
    snprintf(full_path, PATH_MAX, "%s%s", source_dir, path);
}
```

Fungsi ini digunakan untuk menggabungkan path source directory dengan path file yang diminta oleh FUSE.

```bash
Fungsi is_tujuan_file
static int is_tujuan_file(const char *path)
{
    return strcmp(path, "/tujuan.txt") == 0;
}
```

Fungsi ini digunakan untuk mengecek apakah file yang sedang diakses adalah file virtual /tujuan.txt. Jika path sama dengan /tujuan.txt, fungsi akan mengembalikan nilai benar. Jika bukan, fungsi akan mengembalikan nilai salah.

```bash
static void trim_newline(char *str)
{
    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}
```

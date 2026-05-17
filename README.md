# SISOP-4-2026-IT-037

## Struktur Keseluruhan
<img width="450" height="110" alt="Screenshot 2026-05-17 220342" src="https://github.com/user-attachments/assets/fc0b435b-9f61-4c18-8f07-8e9c8b87c360" />

---

# Soal_1 - Save Asisten Kenz

## Penjelasan

File utama: `kenz_rescue.c`

Program dijalankan dengan format:

1. Masuk ke folder soal

   ```bash
   cd ~/SISOP/modul_4/soal_1
   ```

2. Buat folder mount

   ```bash
   mkdir mnt
   ```

3. Masukkan lagi file `amba_files.zip`, lalu extract

   ```bash
   unzip amba_files.zip
   ```

   ```bash
   rm amba_files.zip
   ```

4. Compile program

   ```bash
   gcc kenz_rescue.c -o kenz_rescue -D_FILE_OFFSET_BITS=64 -Wno-format-truncation -lfuse
   ```

5. Jalankan FUSE

   ```bash
   ./kenz_rescue amba_files mnt
   ```

6. Buka terminal baru, lalu masuk lagi ke folder soal

   ```bash
   cd ~/SISOP/modul_4/soal_1
   ```

7. Cek hasil mount

   ```bash
   ls mnt
   ```

8. Cek passthrough salah satu file

   ```bash
   cat mnt/1.txt
   ```

9. Cek passthrough semua file

   ```bash
   for i in 1 2 3 4 5 6 7; do diff mnt/$i.txt amba_files/$i.txt && echo "$i.txt OK"; done
   ```

10. Cek file virtual

    ```bash
    cat mnt/tujuan.txt
    ```

11. Cek `tujuan.txt` tidak ada di source

    ```bash
    ls amba_files
    ```

    ```bash
    ls amba_files/tujuan.txt
    ```

12. Cek ukuran file virtual

    ```bash
    stat mnt/tujuan.txt
    ```

    ```bash
    wc -c mnt/tujuan.txt
    ```

13. Unmount filesystem

    ```bash
    fusermount -u mnt
    ```

---

## Penjelasan `kenz_rescue.c`

```c
#define FUSE_USE_VERSION 28
```

Bagian ini digunakan untuk menentukan versi FUSE yang dipakai, yaitu FUSE versi 28. Hal ini sesuai dengan ketentuan soal bahwa program harus menggunakan `FUSE_USE_VERSION 28`.

```c
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

```c
static char source_dir[PATH_MAX];
```

Variabel `source_dir` digunakan untuk menyimpan path absolut dari folder source. Path ini didapat dari argumen pertama program menggunakan `realpath()`. Dengan menyimpan path absolut, program tetap bisa mengakses file source dengan benar meskipun proses FUSE berjalan dari konteks mount directory.


### Fungsi `make_full_path`

```c
static void make_full_path(char full_path[PATH_MAX], const char *path)
{
    snprintf(full_path, PATH_MAX, "%s%s", source_dir, path);
}
```

Fungsi ini digunakan untuk menggabungkan path source directory dengan path file yang diminta oleh FUSE. Contohnya, ketika user mengakses `/1.txt` di mount directory, program akan mengubahnya menjadi path asli yang berada di dalam folder `amba_files`.

### Fungsi `is_tujuan_file`

```c
static int is_tujuan_file(const char *path)
{
    return strcmp(path, "/tujuan.txt") == 0;
}
```

Fungsi ini digunakan untuk mengecek apakah file yang sedang diakses adalah file virtual `/tujuan.txt`. Jika path sama dengan `/tujuan.txt`, fungsi akan mengembalikan nilai benar. Jika bukan, fungsi akan mengembalikan nilai salah.

### Fungsi `trim_newline`

```c
static void trim_newline(char *str)
{
    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}
```

Fungsi ini digunakan untuk menghapus karakter newline `\n` dan carriage return `\r` di akhir string. Hal ini diperlukan karena fragment dari baris `KOORD:` dibaca menggunakan `fgets()`, sehingga biasanya masih membawa newline di akhir baris.

### Fungsi `generate_tujuan_content`

```c
static void generate_tujuan_content(char *buffer, size_t size)
{
    char gabungan[4096];
    gabungan[0] = '\0';

    for (int i = 1; i <= 7; i++) {
        char file_path[PATH_MAX];
        char line[1024];

        snprintf(file_path, sizeof(file_path), "%s/%d.txt", source_dir, i);

        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
            continue;
        }

        while (fgets(line, sizeof(line), file) != NULL) {
            if (strncmp(line, "KOORD:", 6) == 0) {
                char *fragment = line + 6;

                while (*fragment == ' ' || *fragment == '\t') {
                    fragment++;
                }

                trim_newline(fragment);
                strncat(gabungan, fragment, sizeof(gabungan) - strlen(gabungan) - 1);
                break;
            }
        }

        fclose(file);
    }

    snprintf(buffer, size, "Tujuan Mas Amba: %s\n", gabungan);
}
```

Fungsi ini digunakan untuk membuat isi file virtual `tujuan.txt`. Program membaca file `1.txt` sampai `7.txt` secara berurutan, lalu mencari baris yang diawali dengan prefix `KOORD:`.

Setelah baris `KOORD:` ditemukan, program mengambil fragment setelah prefix tersebut. Spasi atau tab di awal fragment dihapus, kemudian newline di akhir fragment juga dihapus. Semua fragment dari tujuh file tersebut digabungkan tanpa separator tambahan.

Hasil akhirnya dimasukkan ke buffer dengan format:

```txt
Tujuan Mas Amba: <gabungan_fragment>
```

### Callback `kenz_getattr`

```c
static int kenz_getattr(const char *path, struct stat *stbuf)
{
    int result;
    char full_path[PATH_MAX];

    memset(stbuf, 0, sizeof(struct stat));

    if (is_tujuan_file(path)) {
        char content[8192];
        generate_tujuan_content(content, sizeof(content));

        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(content);
        return 0;
    }

    make_full_path(full_path, path);

    result = lstat(full_path, stbuf);
    if (result == -1) {
        return -errno;
    }

    return 0;
}
```

Callback `kenz_getattr` digunakan untuk mengambil atribut file atau direktori. Jika file yang diminta adalah `/tujuan.txt`, atributnya dibuat secara manual karena file tersebut merupakan file virtual.

Pada bagian `tujuan.txt`, program mengatur file sebagai regular file dengan permission read-only `0444`, jumlah link `1`, dan ukuran file sesuai panjang isi yang dibuat oleh `generate_tujuan_content()`.

Jika path bukan `/tujuan.txt`, maka program akan mengambil atribut file asli dari source directory menggunakan `lstat()`.

### Callback `kenz_readdir`

```c
static int kenz_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    DIR *dp;
    struct dirent *de;
    char full_path[PATH_MAX];

    make_full_path(full_path, path);

    dp = opendir(full_path);
    if (dp == NULL) {
        return -errno;
    }

    while ((de = readdir(dp)) != NULL) {
        struct stat st;

        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if (filler(buf, de->d_name, &st, 0)) {
            break;
        }
    }

    if (strcmp(path, "/") == 0) {
        char tujuan_content[8192];
        struct stat st;

        generate_tujuan_content(tujuan_content, sizeof(tujuan_content));

        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFREG | 0444;
        st.st_nlink = 1;
        st.st_size = strlen(tujuan_content);

        filler(buf, "tujuan.txt", &st, 0);
    }

    closedir(dp);
    return 0;
}
```

Callback `kenz_readdir` digunakan saat user menjalankan perintah seperti `ls mnt`. Program membaca isi direktori asli dari source directory menggunakan `opendir()` dan `readdir()`.

Setelah semua file asli dimasukkan ke hasil listing, program menambahkan file virtual `tujuan.txt` jika direktori yang sedang dibaca adalah root directory `/`. Dengan cara ini, `tujuan.txt` muncul di `mnt/`, tetapi tidak muncul di `amba_files/`.

### Callback `kenz_open`

```c
static int kenz_open(const char *path, struct fuse_file_info *fi)
{
    int fd;
    char full_path[PATH_MAX];

    if (is_tujuan_file(path)) {
        if ((fi->flags & O_ACCMODE) != O_RDONLY) {
            return -EACCES;
        }

        return 0;
    }

    make_full_path(full_path, path);

    fd = open(full_path, fi->flags);
    if (fd == -1) {
        return -errno;
    }

    close(fd);
    return 0;
}
```

Callback `kenz_open` digunakan saat file dibuka. Jika file yang dibuka adalah `/tujuan.txt`, maka program hanya mengizinkan akses read-only. Jika file dibuka dengan mode selain read-only, program akan mengembalikan error `-EACCES`.

Jika file yang dibuka bukan `/tujuan.txt`, program akan membuka file asli dari source directory menggunakan `open()`. Jika berhasil, file langsung ditutup kembali menggunakan `close()`.

### Callback `kenz_read`

```c
static int kenz_read(const char *path, char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int result;
    char full_path[PATH_MAX];

    (void) fi;

    if (is_tujuan_file(path)) {
        char content[8192];
        size_t len;

        generate_tujuan_content(content, sizeof(content));
        len = strlen(content);

        if ((size_t) offset < len) {
            if (offset + size > len) {
                size = len - offset;
            }

            memcpy(buf, content + offset, size);
        } else {
            size = 0;
        }

        return size;
    }

    make_full_path(full_path, path);

    fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    result = pread(fd, buf, size, offset);
    if (result == -1) {
        result = -errno;
    }

    close(fd);
    return result;
}
```

Callback `kenz_read` digunakan saat user membaca isi file, misalnya dengan `cat mnt/1.txt` atau `cat mnt/tujuan.txt`.

Jika file yang dibaca adalah `/tujuan.txt`, maka isi file dibuat langsung saat dibaca menggunakan `generate_tujuan_content()`. Karena itu, file `tujuan.txt` bersifat on-the-fly dan tidak disimpan secara fisik di disk.

Jika file yang dibaca bukan `/tujuan.txt`, program membaca file asli dari source directory menggunakan `pread()`. Dengan cara ini, file `1.txt` sampai `7.txt` dapat dibaca secara passthrough.

### Struct `fuse_operations`

```c
static struct fuse_operations kenz_oper = {
    .getattr = kenz_getattr,
    .readdir = kenz_readdir,
    .open    = kenz_open,
    .read    = kenz_read,
};
```

Struct ini digunakan untuk mendaftarkan callback FUSE yang dipakai oleh program. Operasi `getattr`, `readdir`, `open`, dan `read` akan diarahkan ke fungsi masing-masing yang sudah dibuat.

### Fungsi `main`

```c
int main(int argc, char *argv[])
{
    char *fuse_argv[2];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <mount_directory>\n", argv[0]);
        return 1;
    }

    if (realpath(argv[1], source_dir) == NULL) {
        perror("realpath");
        return 1;
    }

    fuse_argv[0] = argv[0];
    fuse_argv[1] = argv[2];

    return fuse_main(2, fuse_argv, &kenz_oper, NULL);
}
```

Fungsi `main` digunakan sebagai titik awal program. Program mengecek apakah jumlah argumen sudah tepat, yaitu dua argumen berupa `source_directory` dan `mount_directory`.

Saat argumen tidak sesuai, program menampilkan format penggunaan:

```bash
Usage: ./kenz_rescue <source_directory> <mount_directory>
```

Kemudian program menyimpan path absolut dari `source_directory` menggunakan `realpath()`. Setelah itu, program menjalankan `fuse_main()` dengan mount directory sebagai argumen FUSE.

---

## OUTPUT
1. `gcc kenz_rescue.c -o kenz_rescue -D_FILE_OFFSET_BITS=64 -Wno-format-truncation -lfuse`

<img width="919" height="85" alt="image" src="https://github.com/user-attachments/assets/01f8e9d6-2434-4c7f-9bd9-c53576f088c1" />

2. Untuk menjalankan FUSE `./kenz_rescue amba_files mnt`

<img width="775" height="77" alt="image" src="https://github.com/user-attachments/assets/4ce79837-d2c2-47fc-88eb-9bfad0fe1f94" />

3. Untuk mengecek isi mount directory `ls mnt`

<img width="721" height="83" alt="image" src="https://github.com/user-attachments/assets/f740946d-11ff-496a-8618-0c924faeb44d" />

4. Untuk mengecek isi file passthrough `cat mnt/1.txt`

<img width="918" height="715" alt="image" src="https://github.com/user-attachments/assets/6447e68c-227d-4d17-9902-8f3af6a18021" />

5. Untuk mengecek semua file passthrough sama persis `for i in 1 2 3 4 5 6 7; do diff mnt/$i.txt amba_files/$i.txt && echo "$i.txt OK"; done`

<img width="920" height="218" alt="image" src="https://github.com/user-attachments/assets/a04f29d9-7e11-4ec7-a285-3f443360e9cb" />

6. Untuk mengecek isi file virtual tujuan.txt `cat mnt/tujuan.txt`

<img width="718" height="70" alt="image" src="https://github.com/user-attachments/assets/6be5e43a-55b8-4874-8966-91f21105ef4d" />

9. Untuk mengecek tujuan.txt tidak ada di source directory `ls amba_files`
    `ls amba_files/tujuan.txt`

<img width="732" height="128" alt="image" src="https://github.com/user-attachments/assets/7442a911-3cf5-47ec-ac56-73ca846716f3" />

10. Untuk mengecek ukuran file virtual `stat mnt/tujuan.txt`
    `wc -c mnt/tujuan.txt`

<img width="845" height="278" alt="image" src="https://github.com/user-attachments/assets/be2e0dfd-e341-47b3-9afe-f93c2c09e4b2" />

11. Untuk melihat struktur mount directory `tree`

<img width="801" height="528" alt="image" src="https://github.com/user-attachments/assets/cfbd8190-571b-4fb4-abbb-65e52462ac2b" />

12. Untuk unmount filesystem `fusermount -u mnt`

<img width="758" height="55" alt="image" src="https://github.com/user-attachments/assets/c5854ad0-4c36-4438-8966-f09d7dde13d4" />

13. `ls mnt` setelah di unmount

<img width="656" height="58" alt="Screenshot 2026-05-17 221410" src="https://github.com/user-attachments/assets/1b23493a-39ad-49bb-8d5b-12de0bc381e6" />

---
## Kendala

Tidak ada.

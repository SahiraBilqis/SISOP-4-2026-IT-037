#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

static char source_dir[PATH_MAX];

static void make_full_path(char fullpath[PATH_MAX], const char *path)
{
    snprintf(fullpath, PATH_MAX, "%s%s", source_dir, path);
}

static int is_tujuan_file(const char *path)
{
    return strcmp(path, "/tujuan.txt") == 0;
}

static void trim_newline(char *str)
{
    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static void generate_tujuan_content(char *buffer, size_t size)
{
    char result[8192];
    result[0] = '\0';

    for (int i = 1; i <= 7; i++) {
        char filepath[PATH_MAX];
        char line[4096];

        snprintf(filepath, sizeof(filepath), "%s/%d.txt", source_dir, i);

        FILE *fp = fopen(filepath, "r");
        if (fp == NULL) {
            continue;
        }

        while (fgets(line, sizeof(line), fp) != NULL) {
            if (strncmp(line, "KOORD:", 6) == 0) {
                char *fragment = line + 6;

                while (*fragment == ' ' || *fragment == '\t') {
                    fragment++;
                }

                trim_newline(fragment);

                strncat(result, fragment, sizeof(result) - strlen(result) - 1);
                break;
            }
        }

        fclose(fp);
    }

    snprintf(buffer, size, "Tujuan Mas Amba: %s\n", result);
}

static int kenz_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fullpath[PATH_MAX];

    memset(stbuf, 0, sizeof(struct stat));

    if (is_tujuan_file(path)) {
        char content[8192];

        generate_tujuan_content(content, sizeof(content));

        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(content);

        return 0;
    }

    make_full_path(fullpath, path);

    res = lstat(fullpath, stbuf);
    if (res == -1) {
        return -errno;
    }

    return 0;
}

static int kenz_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    char fullpath[PATH_MAX];

    (void) offset;
    (void) fi;

    make_full_path(fullpath, path);

    dp = opendir(fullpath);
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
        struct stat st;

        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFREG | 0444;
        st.st_nlink = 1;

        filler(buf, "tujuan.txt", &st, 0);
    }

    closedir(dp);

    return 0;
}

static int kenz_open(const char *path, struct fuse_file_info *fi)
{
    int res;
    char fullpath[PATH_MAX];

    if (is_tujuan_file(path)) {
        if ((fi->flags & 3) != O_RDONLY) {
            return -EACCES;
        }

        return 0;
    }

    make_full_path(fullpath, path);

    res = open(fullpath, fi->flags);
    if (res == -1) {
        return -errno;
    }

    close(res);

    return 0;
}

static int kenz_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
    int fd;
    int res;
    char fullpath[PATH_MAX];

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

    make_full_path(fullpath, path);

    fd = open(fullpath, O_RDONLY);
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

static struct fuse_operations kenz_oper = {
    .getattr = kenz_getattr,
    .readdir = kenz_readdir,
    .open    = kenz_open,
    .read    = kenz_read,
};

int main(int argc, char *argv[])
{
    char resolved_source[PATH_MAX];
    char *fuse_argv[3];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <mount_directory>\n", argv[0]);
        return 1;
    }

    if (realpath(argv[1], resolved_source) == NULL) {
        perror("realpath");
        return 1;
    }

    strncpy(source_dir, resolved_source, PATH_MAX - 1);
    source_dir[PATH_MAX - 1] = '\0';

    fuse_argv[0] = argv[0];
    fuse_argv[1] = argv[2];
    fuse_argv[2] = NULL;

    return fuse_main(2, fuse_argv, &kenz_oper, NULL);
}
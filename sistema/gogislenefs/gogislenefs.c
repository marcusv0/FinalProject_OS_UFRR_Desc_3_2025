#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static const char *file_path = "/hello.txt";
static const char *file_content = "Este é o Gogislenefs!\n";

static int gogislenefs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, file_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(file_content);
    } else
        return -ENOENT;
    return 0;
}

static int gogislenefs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, file_path + 1, NULL, 0, 0);

    return 0;
}

static int gogislenefs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, file_path) != 0)
        return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int gogislenefs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    size_t len;
    (void) fi;
    if (strcmp(path, file_path) != 0)
        return -ENOENT;

    len = strlen(file_content);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, file_content + offset, size);
    } else
        size = 0;

    return size;
}

static const struct fuse_operations operations = {
    .getattr = gogislenefs_getattr,
    .readdir = gogislenefs_readdir,
    .open = gogislenefs_open,
    .read = gogislenefs_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &operations, NULL);
}


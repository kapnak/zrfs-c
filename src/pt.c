#define FUSE_USE_VERSION 31

#include "pt.h"
#include "zrfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fuse3/fuse.h>

#define MAX_READ_SIZE 1000000
#define MAX_WRITE_SIZE 1000000

#if 1
    #define log(fmt, ...) \
    printf("%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#else
    #define debug(fmt, ...) ;
#endif

#define concat_path(ptfs, fn, fp)       (sizeof fp > (unsigned)snprintf(fp, sizeof fp, "%s%s", ptfs->rootdir, fn))

#define fi_dirbit                       (0x8000000000000000ULL)
#define fi_fh(fi, MASK)                 ((fi)->fh & (MASK))
#define fi_setfh(fi, FH, MASK)          ((fi)->fh = (intptr_t)(FH) | (MASK))
#define fi_fd(fi)                       (fi_fh(fi, fi_dirbit) ? \
    dirfd((DIR *)(intptr_t)fi_fh(fi, ~fi_dirbit)) : (int)fi_fh(fi, ~fi_dirbit))
#define fi_dirp(fi)                     ((DIR *)(intptr_t)fi_fh(fi, ~fi_dirbit))
#define fi_setfd(fi, fd)                (fi_setfh(fi, fd, 0))
#define fi_setdirp(fi, dirp)            (fi_setfh(fi, dirp, fi_dirbit))

#define ptfs_impl_fullpath(n)           \
    char full ## n[PATH_MAX * 4];           \
    if (!concat_path(((PTFS *)fuse_get_context()->private_data), n, full ## n))\
    return -ENAMETOOLONG;           \
    n = full ## n

typedef struct {
    const char *rootdir;
} PTFS;

RemotePeer server;


static int ptfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    log("[GETATTR]\n");
    if (fi == 0) {
        ptfs_impl_fullpath(path);
        return z_lstat(&server, path, stbuf);
    } else {
        int fd = fi->fh;
        return z_fstat(&server, fd, stbuf);
    }
}


static int ptfs_mkdir(const char *path, mode_t mode) {
    log("[MKDIR]\n");
    ptfs_impl_fullpath(path);
    return z_mkdir(&server, path);
}


static int ptfs_unlink(const char *path) {
    log("[UNLINK]\n");
    ptfs_impl_fullpath(path);
    return z_unlink(&server, path);
}


static int ptfs_rmdir(const char *path) {
    log("[RMDIR]\n");
    ptfs_impl_fullpath(path);
    return z_rmdir(&server, path);
}


static int ptfs_rename(const char *oldpath, const char *newpath, unsigned int flags) {
    log("[RENAME]\n");
    ptfs_impl_fullpath(oldpath);
    ptfs_impl_fullpath(newpath);
    return z_rename(&server, oldpath, newpath);
}


static int ptfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    log("[TRUNCATE]\n");
    ptfs_impl_fullpath(path);
    if (fi == 0) {
        return z_truncate(&server, path, size);
    } else {
        int fd = fi_fd(fi);
        return z_ftruncate(&server, fd, size);
    }
}


static int ptfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    log("[CREATE]\n");
    ptfs_impl_fullpath(path);
    int fd = z_create(&server, path, fi->flags);
    if (fd < 0)
        return fd;
    fi_setfd(fi, fd);
    return 0;
}


static int ptfs_open(const char *path, struct fuse_file_info *fi) {
    log("[OPEN]\n");
    ptfs_impl_fullpath(path);
    int fd = z_open(&server, path, fi->flags);
    if (fd < 0)
        return fd;
    fi_setfd(fi, fd);
    return 0;
}


static int ptfs_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
    log("[READ]\n");
    ptfs_impl_fullpath(path);
    int fd = fi_fd(fi);

    ssize_t total_read = 0;
    while (size > 0) {
        size_t chunk_size = (size > MAX_READ_SIZE) ? MAX_READ_SIZE : size;
        ssize_t nb = z_pread(&server, fd, buf + total_read, chunk_size, off + total_read);
        if (nb < 0)
            return nb;
        else if (nb == 0)
            break;
        total_read += nb;
        size -= nb;
        if ((size_t)nb < chunk_size)
            break;
    }
    return total_read;
}


static int ptfs_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
    log("[WRITE] (path: %s, size: %ld, offset: %ld)\n", path, size, off);
    ptfs_impl_fullpath(path);
    int fd = fi_fd(fi);

    ssize_t total_written = 0;
    while (size > 0) {
        size_t chunk_size = (size > MAX_WRITE_SIZE) ? MAX_WRITE_SIZE : size;
        ssize_t nb = z_pwrite(&server, fd, buf + total_written, chunk_size, off + total_written);
        if (nb < 0)
            return nb;
        total_written += nb;
        size -= nb;
        if ((size_t)nb < chunk_size)
            break;
    }
    return total_written;
}


static int ptfs_release(const char *path, struct fuse_file_info *fi) {
    log("[RELEASE]\n");
    ptfs_impl_fullpath(path);
    int fd = fi_fd(fi);
    return z_release(&server, fd);
}


static int ptfs_opendir(const char *path, struct fuse_file_info *fi) {
    log("[OPENDIR]\n");
    ptfs_impl_fullpath(path);
    fi->fh = (uint64_t) malloc(strlen(path) + 1);
    strcpy((char *)fi->fh, path);
    return 0;
}


static int ptfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    log("[READDIR]\n");
    ptfs_impl_fullpath(path);
    int dirs_length;
    z_dirent *dirs;
    int res = z_readdir(&server, &dirs, &dirs_length, (char *)fi->fh);

    if (res != 0)
        return res;

    for (int i = 0; i < dirs_length; i++) {
        enum fuse_fill_dir_flags fill_flags = 0;
        if (dirs[i].fill_dir_plus)
            fill_flags |= FUSE_FILL_DIR_PLUS;
        if (filler(buf, dirs[i].d_name, &dirs[i].st, 0, fill_flags))
            break;
    }
    free(dirs);
    return 0;
}


static int ptfs_releasedir(const char *path, struct fuse_file_info *fi) {
    log("[RELEASEDIR]\n");
    ptfs_impl_fullpath(path);
    free((char *)fi->fh);
    return 0;
}


static int ptfs_statfs(const char *path, struct statvfs *stbuf) {
    log("[STATFS]\n");
    ptfs_impl_fullpath(path);
    return z_statvfs(&server, path, stbuf);
}


static int ptfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    return -ENOTSUP;
}


static void *ptfs_init(struct fuse_conn_info *conn, struct fuse_config *conf) {
    conf->use_ino = 1;
    conf->nullpath_ok = 1;
    conf->entry_timeout = 0;
    conf->attr_timeout = 0;
    conf->negative_timeout = 0;

    conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);

    #if defined(FSP_FUSE_CAP_CASE_INSENSITIVE)  // TODO : Does this remove caps in setattr ?
    conn->want |= (conn->capable & FSP_FUSE_CAP_CASE_INSENSITIVE);
    #endif

    return fuse_get_context()->private_data;
}


static struct fuse_operations ptfs_ops = {
    .getattr = ptfs_getattr,
    .mkdir = ptfs_mkdir,
    .unlink = ptfs_unlink,
    .rmdir = ptfs_rmdir,
    .rename = ptfs_rename,
    .truncate = ptfs_truncate,
    .open = ptfs_open,
    .read = ptfs_read,
    .write = ptfs_write,
    .release = ptfs_release,
    .opendir = ptfs_opendir,
    .readdir = ptfs_readdir,
    .releasedir = ptfs_releasedir,
    .init = ptfs_init,
    .create = ptfs_create,
    .statfs = ptfs_statfs,
    .utimens = ptfs_utimens
};


int z_mount(char *ip, int port,
            char *path,
            unsigned char pk[ED25519_PK_LENGTH],
            unsigned char sk[ED25519_SK_LENGTH],
            unsigned char server_pk[ED25519_PK_LENGTH]) {
    printf("Connecting to %s:%d ...\n", ip, port);
    quick_connect(&server, pk, sk, server_pk, ip, port, NULL);
    printf("Connected !\n");
    printf("Mounting on '%s' ...\n", path);
    PTFS ptfs = { 0 };
    ptfs.rootdir = ".";
    umask(0);

    if (__CYGWIN__) {
        char *argv[] = {"zrfs", path, "-f", "-o", "uid=-1,gid=-1", NULL}; //"-f", "-d", NULL}; // allow_other,defer_permissions,umask=000
        int argc = 5;
        return fuse_main(argc, argv, &ptfs_ops, &ptfs);
    } else {
        char *argv[] = {"zrfs", path, "-f", NULL}; //"-f", "-d", NULL}; // allow_other,defer_permissions,umask=000
        int argc = 3;
        return fuse_main(argc, argv, &ptfs_ops, &ptfs);
    }
}

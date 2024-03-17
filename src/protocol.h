#include <stdint.h>

#define REQ_GETATTR                 11
#define REQ_GETATTR_FH(message)     (*(uint64_t *)(message+1))
#define REQ_GETATTR_PATH(message)   ((char *)(message+9))
#define REP_GETATTR_LENGTH          56
#define REP_GETATTR_ERR(reply)      (*(int64_t *)(reply))
#define REP_GETATTR_DEV(reply)      (*(uint64_t *)(reply+8))
#define REP_GETATTR_INO(reply)      (*(uint64_t *)(reply+16))
#define REP_GETATTR_SIZE(reply)     (*(int64_t *)(reply+24))
#define REP_GETATTR_BLKSIZE(reply)  (*(uint64_t *)(reply+32))
#define REP_GETATTR_BLOCKS(reply)   (*(int64_t *)(reply+40))
#define REP_GETATTR_MODE(reply)     (*(uint64_t *)(reply+48))
// #define REP_GETATTR_ATIME(reply)    (*(uint64_t *)(reply+384))
// #define REP_GETATTR_MTIME(reply)    (*(uint64_t *)(reply+448))
// #define REP_GETATTR_CTIME(reply)    (*(uint64_t *)(reply+512))

#define REQ_ACCESS                  12
#define REQ_ACCESS_PATH(message)    ((char *)(message+1))
#define REP_ACCESS_LENGTH           8
#define REP_ACCESS_ERR(reply)       (*(int64_t *)(reply))

#define REQ_READDIR                 13
#define REQ_READDIR_PATH(message)   ((char *)(message+1))
#define REP_READDIR_ERR(reply)      (*(int64_t *)(reply))
#define REP_ENTRY_FDP(entry)        (*(char *)(entry))
#define REP_ENTRY_DEV(entry)        (*(uint64_t *)(entry+1))
#define REP_ENTRY_INO(entry)        (*(uint64_t *)(entry+9))
#define REP_ENTRY_SIZE(entry)       (*(int64_t *)(entry+17))
#define REP_ENTRY_BLKSIZE(entry)    (*(uint64_t *)(entry+25))
#define REP_ENTRY_BLOCKS(entry)     (*(int64_t *)(entry+33))
#define REP_ENTRY_MODE(entry)       (*(uint64_t *)(entry+41))
#define REP_ENTRY_NAME(entry)       ((char *)(entry+49))

#define REQ_MKDIR                   14
#define REQ_MKDIR_PATH(message)     ((char *)(message+1))
#define REP_MKDIR_LENGTH            8
#define REP_MKDIR_ERR(reply)        (*(int64_t *)(reply))

#define REQ_UNLINK                  15
#define REQ_UNLINK_PATH(message)    ((char *)(message+1))
#define REP_UNLINK_LENGTH           8
#define REP_UNLINK_ERR(reply)       (*(int64_t *)(reply))

#define REQ_RMDIR                   16
#define REQ_RMDIR_PATH(message)     ((char *)(message+1))
#define REP_RMDIR_LENGTH            8
#define REP_RMDIR_ERR(reply)        (*(int64_t *)(reply))

#define REQ_SYMLINK                 17
#define REQ_SYMLINK_FROM(message)   ((char *)(message+1))
#define REQ_SYMLINK_TO(message)     ((char *)(message+2+strlen(REQ_SYMLINK_FROM(message))))
#define REP_SYMLINK_LENGTH          8
#define REP_SYMLINK_ERR(reply)      (*(int64_t *)(reply))

#define REQ_RENAME                  18
#define REQ_RENAME_FROM(message)    ((char *)(message+1))
#define REQ_RENAME_TO(message)      ((char *)(message+2+strlen(REQ_RENAME_FROM(message))))
#define REP_RENAME_LENGTH           8
#define REP_RENAME_ERR(reply)       (*(int64_t *)(reply))

#define REQ_LINK                    19
#define REQ_LINK_FROM(message)      ((char *)(message+1))
#define REQ_LINK_TO(message)        ((char *)(message+2+strlen(REQ_LINK_FROM(message))))
#define REP_LINK_LENGTH             8
#define REP_LINK_ERR(reply)         (*(int64_t *)(reply))

#define REQ_CREATE                  20
#define REQ_CREATE_FLAG(message)    (*(int64_t *)(message+1))
#define REQ_CREATE_PATH(message)    ((char *)(message+9))
#define REP_CREATE_LENGTH           16
#define REP_CREATE_ERR(reply)       (*(int64_t *)(reply))
#define REP_CREATE_FH(reply)        (*(int64_t *)(reply+8))

#define REQ_OPEN                    21
#define REQ_OPEN_FLAG(message)      (*(int64_t *)(message+1))
#define REQ_OPEN_PATH(message)      ((char *)(message+9))
#define REP_OPEN_LENGTH             16
#define REP_OPEN_ERR(reply)         (*(int64_t *)(reply))
#define REP_OPEN_FH(reply)          (*(int64_t *)(reply+8))

#define REQ_READ                    22
#define REQ_READ_LENGTH             25
#define REQ_READ_FH(message)        (*(int64_t *)(message+1))
#define REQ_READ_SIZE(message)      (*(uint64_t *)(message+9))
#define REQ_READ_OFFSET(message)    (*(int64_t *)(message+17))
#define REP_READ_ERR(reply)         (*(int64_t *)(reply))
#define REP_READ_BUF(reply)         ((unsigned char *)(reply+8))

#define REQ_WRITE                   23
#define REQ_WRITE_FH(message)       (*(int64_t *)(message+1))
#define REQ_WRITE_SIZE(message)     (*(uint64_t *)(message+9))
#define REQ_WRITE_OFFSET(message)   (*(int64_t *)(message+17))
#define REQ_WRITE_BUF(message)      ((void *)(message+25))
#define REP_WRITE_LENGTH            8
#define REP_WRITE_ERR(reply)        (*(int64_t *)(reply))

#define REQ_STATFS                  24
#define REQ_STATFS_PATH(message)    ((char *)(message+1))
#define REP_STATFS_LENGTH           96
#define REP_STATFS_ERR(reply)       (*(uint64_t *)(reply))
#define REP_STATFS_BSIZE(reply)     (*(uint64_t *)(reply+8))
#define REP_STATFS_FRSIZE(reply)    (*(uint64_t *)(reply+16))
#define REP_STATFS_BLOCKS(reply)    (*(uint64_t *)(reply+24))
#define REP_STATFS_BFREE(reply)     (*(uint64_t *)(reply+32))
#define REP_STATFS_BAVAIL(reply)    (*(uint64_t *)(reply+40))
#define REP_STATFS_FILES(reply)     (*(uint64_t *)(reply+48))
#define REP_STATFS_FFREE(reply)     (*(uint64_t *)(reply+56))
#define REP_STATFS_FAVAIL(reply)    (*(uint64_t *)(reply+64))
#define REP_STATFS_FSID(reply)      (*(uint64_t *)(reply+72))
#define REP_STATFS_FLAG(reply)      (*(uint64_t *)(reply+80))
#define REP_STATFS_NAMEMAX(reply)   (*(uint64_t *)(reply+88))

#define REQ_RELEASE                 25
#define REQ_RELEASE_LENGTH          9
#define REQ_RELEASE_FH(message)     (*(int64_t *)(message+1))
#define REP_RELEASE_LENGTH          8
#define REP_RELEASE_ERR(reply)      (*(uint64_t *)(reply))

#define REQ_GETPERM                 26
#define REQ_GETPERM_PATH(message)   ((char *)(message+1))
#define REP_GETPERM_ERR(reply)      (*(int64_t *)(reply))
#define REP_GETPERM_PERM(reply)     ((char *)(reply+8))

#define REQ_SETPERM                 27
#define REQ_SETPERM_PERM(message)   (*(char *)(message+1))
#define REQ_SETPERM_PK(message)     ((char *)(message+9))
#define REQ_SETPERM_PATH(message)   ((char *)(message+9+PK_BS64_LENGTH))
#define REP_SETPERM_LENGTH          8
#define REP_SETPERM_ERR(reply)      (*(int64_t *)(reply))

#define REQ_TRUNCATE                28
#define REQ_TRUNCATE_FH(message)    (*(int64_t *)(message+1))
#define REQ_TRUNCATE_LENGTH(message) (*(int64_t *)(message+9))
#define REQ_TRUNCATE_PATH(message)  ((char *)(message+17))
#define REP_TRUNCATE_LENGTH         8
#define REP_TRUNCATE_ERR(reply)     (*(int64_t *)(reply))
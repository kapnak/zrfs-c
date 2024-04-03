#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "server.h"
#include "protocol.h"
#include "acl.h"


#define CHECK_PERMISSION(path, permission)                      \
    if (memcmp(peer->pk, peer->local_peer->pk, ED25519_PK_LENGTH) != 0 &&          \
        get_permission(path, peer->attribute) < permission) {   \
                                                                \
        unsigned char reply_err[8];                             \
        *(int64_t *)(reply_err) = EACCES;                       \
        z_reply(peer, reply_err, 8, message->id);               \
        return;                                                 \
    }

/*
char actualrealpath[255];   \
char *ptractualpath = realpath(path, actualrealpath);   \
printf("PERMISSION REFUSED FOR '%s' with > %d aka. [%s]\n", path, permission, actualrealpath); \
*/

typedef struct peer_attributes {
    char pk_bs64[PK_BS64_LENGTH];
    int *opened_fd;
} peer_attributes;



void connection_listener(RemotePeer *peer) {
    (void) peer;
    peer->attribute = (char *)malloc(PK_BS64_LENGTH);
    z_helpers_bin_to_bs32(peer->pk, ED25519_PK_LENGTH, peer->attribute, PK_BS32_LENGTH);
    printf("[+] Client '%s' is now connected.\n", (char *)peer->attribute);
}


void disconnection_listener(RemotePeer *peer) {
    (void) peer;
    printf("[-] Peer disconnected.\n");
    free(peer->attribute);
}


void message_listener(RemotePeer *peer, Message *message) {
    switch (message->content[0]) {

        case REQ_GETATTR: {
            int res;
            struct stat st;
            unsigned char reply[REP_GETATTR_LENGTH] = {0};

            if (REQ_GETATTR_FH(message->content))
                res = fstat(REQ_GETATTR_FH(message->content), &st);
            else {
                CHECK_PERMISSION(REQ_GETATTR_PATH(message->content), ACL_REFERENCE)
                res = lstat(REQ_GETATTR_PATH(message->content), &st);
            }

            if (res == -1)
                REP_GETATTR_ERR(reply) = errno;
            else {
                REP_GETATTR_ERR(reply) = 0;
                REP_GETATTR_DEV(reply) = st.st_dev;
                REP_GETATTR_INO(reply) = st.st_ino;
                REP_GETATTR_SIZE(reply) = st.st_size;
                REP_GETATTR_BLKSIZE(reply) = st.st_blksize;
                REP_GETATTR_BLOCKS(reply) = st.st_blocks;
                REP_GETATTR_MODE(reply) = st.st_mode & S_IFMT;
                REP_GETATTR_MODE(reply) = REP_GETATTR_MODE(reply) | S_IRWXU | S_IRWXG | S_IRWXO;
            }

            z_reply(peer, reply, REP_GETATTR_LENGTH, message->id);
            break;
        }

        case REQ_ACCESS: {
            CHECK_PERMISSION(REQ_ACCESS_PATH(message->content), ACL_REFERENCE)
            unsigned char reply[REP_ACCESS_LENGTH];
            REP_ACCESS_ERR(reply) =
                    access(REQ_ACCESS_PATH(message->content), F_OK) == -1 ? errno : 0;
            z_reply(peer, reply, REP_ACCESS_LENGTH, message->id);
            break;
        }

        case REQ_READDIR: {
            CHECK_PERMISSION(REQ_READDIR_PATH(message->content), ACL_READ)
            DIR *dir;
            size_t reply_length = 8;
            unsigned char *reply = malloc(reply_length);

            dir = opendir(REQ_READDIR_PATH(message->content));
            if (dir == NULL) {
                *(int64_t *) reply = errno;
            } else {
                struct dirent *entry;
                size_t offset = reply_length;
                REP_READDIR_ERR(reply) = 0;
                while ((entry = readdir(dir)) != NULL) {
                    struct stat st;
                    reply_length += 50 + strlen(entry->d_name);    // 1 (fdp) + 6*8 (st) + 1 (\0 of str)
                    reply = realloc(reply, reply_length);
                    unsigned char *reply_entry = reply + offset;
                    offset = reply_length;

                    if (fstatat(dirfd(dir), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) != -1) {
                        REP_ENTRY_FDP(reply_entry) = 1;
                        REP_ENTRY_DEV(reply_entry) = st.st_dev;
                        REP_ENTRY_INO(reply_entry) = st.st_ino;
                        REP_ENTRY_SIZE(reply_entry) = st.st_size;
                        REP_ENTRY_BLKSIZE(reply_entry) = st.st_blksize;
                        REP_ENTRY_BLOCKS(reply_entry) = st.st_blocks;
                        REP_ENTRY_MODE(reply_entry) = st.st_mode & S_IFMT;
                        REP_ENTRY_MODE(reply_entry) = REP_ENTRY_MODE(reply_entry) | S_IRWXU | S_IRWXG | S_IRWXO;
                    } else {
                        printf("[ERROR] fstatat for fill_dir_plus failed for '%s'.\n", entry->d_name);
                        perror("failed");
                        REP_ENTRY_FDP(reply_entry) = 0;
                        REP_ENTRY_INO(reply_entry) = entry->d_ino;
                        REP_ENTRY_MODE(reply_entry) = entry->d_type << 12;
                        REP_ENTRY_MODE(reply_entry) = REP_ENTRY_MODE(reply_entry) | S_IRWXU | S_IRWXG | S_IRWXO;
                    }
                    strcpy(REP_ENTRY_NAME(reply_entry), entry->d_name);
                }
            }
            closedir(dir);
            z_reply(peer, reply, reply_length, message->id);
            free(reply);
            break;
        }

        case REQ_MKDIR: {
            char *pathdup = strdup(REQ_MKDIR_PATH(message->content));
            char *parent = dirname(pathdup);
            CHECK_PERMISSION(parent, ACL_WRITE)
            free(pathdup);
            unsigned char reply[REP_MKDIR_LENGTH];
            REP_MKDIR_ERR(reply) =
                    mkdir(REQ_MKDIR_PATH(message->content), 0777) == -1 ? errno : 0;
            z_reply(peer, reply, REP_MKDIR_LENGTH, message->id);
            break;
        }

        case REQ_UNLINK: {
            unsigned char reply[REP_UNLINK_LENGTH];
            REP_UNLINK_ERR(reply) =
                    unlink(REQ_UNLINK_PATH(message->content)) == -1 ? errno : 0;
            z_reply(peer, reply, REP_UNLINK_LENGTH, message->id);
            break;
        }

        case REQ_RMDIR: {
            CHECK_PERMISSION(REQ_RMDIR_PATH(message->content), ACL_WRITE)
            unsigned char reply[REP_RMDIR_LENGTH];
            REP_RMDIR_ERR(reply) =
                    rmdir(REQ_RMDIR_PATH(message->content)) == -1 ? errno : 0;
            z_reply(peer, reply, REP_RMDIR_LENGTH, message->id);
            break;
        }

        case REQ_SYMLINK: {
            unsigned char reply[REP_SYMLINK_LENGTH];
            REP_SYMLINK_ERR(reply) =
                    symlink(REQ_SYMLINK_FROM(message->content), REQ_SYMLINK_TO(message->content)) == -1 ? errno : 0;
            z_reply(peer, reply, REP_SYMLINK_LENGTH, message->id);
            break;
        }

        case REQ_RENAME: {
            CHECK_PERMISSION(REQ_RENAME_FROM(message->content), ACL_WRITE)
            unsigned char reply[REP_RENAME_LENGTH];
            REP_RENAME_ERR(reply) =
                    rename(REQ_RENAME_FROM(message->content), REQ_RENAME_TO(message->content)) == -1 ? errno : 0;
            z_reply(peer, reply, REP_RENAME_LENGTH, message->id);
            break;
        }

        case REQ_LINK: {
            unsigned char reply[REP_LINK_LENGTH];
            REP_LINK_ERR(reply) =
                    link(REQ_LINK_FROM(message->content), REQ_LINK_TO(message->content)) == -1 ? errno : 0;
            z_reply(peer, reply, REP_LINK_LENGTH, message->id);
            break;
        }

        case REQ_OPEN:
        case REQ_CREATE: {
            unsigned char reply[REP_CREATE_LENGTH];
            if (((message->content[0] == REQ_CREATE) || (REQ_CREATE_FLAG(message->content) & O_WRONLY) || (REQ_CREATE_FLAG(message->content) & O_RDWR))) {
                CHECK_PERMISSION(REQ_CREATE_PATH(message->content), ACL_WRITE)
            } else {
                CHECK_PERMISSION(REQ_OPEN_PATH(message->content), ACL_READ)
            }

            int fd = message->content[0] == REQ_CREATE ?
                     open(REQ_CREATE_PATH(message->content), REQ_CREATE_FLAG(message->content) | O_CREAT, 0777):
                     open(REQ_OPEN_PATH(message->content), REQ_OPEN_FLAG(message->content));
            if (fd == -1)
                REP_CREATE_ERR(reply) = errno;
            else {
                REP_CREATE_ERR(reply) = 0;
                REP_CREATE_FH(reply) = fd;
            }
            z_reply(peer, reply, REP_CREATE_LENGTH, message->id);
            break;
        }

        case REQ_READ: {
            // TODO : Check REQ_READ_SIZE to not let the client allocate 999Gb of RAM.
            unsigned char reply[REQ_READ_SIZE(message->content)+8];
            ssize_t size = pread(REQ_READ_FH(message->content), REP_READ_BUF(reply), REQ_READ_SIZE(message->content), REQ_READ_OFFSET(message->content));
            if (size == -1) {
                REP_READ_ERR(reply) = errno;
                z_reply(peer, reply, 8, message->id);
            } else {
                REP_READ_ERR(reply) = 0;
                z_reply(peer, reply, size + 8, message->id);
            }
            break;
        }

        case REQ_WRITE: {
            if ((REQ_WRITE_SIZE(message->content)+24) > message->content_length) {
                uint64_t reply = -EINVAL;
                z_reply(peer, (unsigned char *)reply, 8, message->id);
                break;
            }
            unsigned char reply[REP_WRITE_LENGTH];
            ssize_t size = pwrite(REQ_WRITE_FH(message->content),
                                  REQ_WRITE_BUF(message->content),
                                  REQ_WRITE_SIZE(message->content),
                                  REQ_WRITE_OFFSET(message->content));

            REP_WRITE_ERR(reply) = size == -1 ? -errno : size;  // TODO : Reverse errno compared to the others :/ that's bad.
            z_reply(peer, reply, REP_WRITE_LENGTH, message->id);
            break;
        }

        case REQ_STATFS: {
            unsigned char reply[REP_STATFS_LENGTH];
            struct statvfs stat;
            REP_STATFS_ERR(reply) = statvfs(REQ_STATFS_PATH(message->content), &stat) == -1 ? errno : 0;
            if (REP_STATFS_ERR(reply) == 0) {
                REP_STATFS_BSIZE(reply) = stat.f_bsize;
                REP_STATFS_FRSIZE(reply) = stat.f_frsize;
                REP_STATFS_BLOCKS(reply) = stat.f_blocks;
                REP_STATFS_BFREE(reply) = stat.f_bfree;
                REP_STATFS_BAVAIL(reply) = stat.f_bavail;
                REP_STATFS_FILES(reply) = stat.f_files;
                REP_STATFS_FFREE(reply) = stat.f_ffree;
                REP_STATFS_FAVAIL(reply) = stat.f_favail;
                REP_STATFS_FSID(reply) = stat.f_fsid;
                REP_STATFS_FLAG(reply) = stat.f_flag;
                REP_STATFS_NAMEMAX(reply) = stat.f_namemax;
            } else {
                printf("Trying to get statfs of '%s' but failed with %d :(\n", REQ_STATFS_PATH(message->content), errno);
                perror("failed");
            }
            z_reply(peer, reply, REP_STATFS_LENGTH, message->id);
            break;
        }

        case REQ_RELEASE: {
            unsigned char reply[REP_RELEASE_LENGTH];
            REP_RELEASE_ERR(reply) =
                    close(REQ_RELEASE_FH(message->content)) == -1 ? errno : 0;
            z_reply(peer, reply, REP_RELEASE_LENGTH, message->id);
            break;
        }

        case REQ_GETPERM: {
            CHECK_PERMISSION(REQ_GETPERM_PATH(message->content), ACL_REFERENCE)
            size_t length;
            char *permissions = get_permissions(REQ_GETPERM_PATH(message->content), &length);
            if (permissions == NULL) {
                uint64_t err = errno;
                z_reply(peer, &err, 8, message->id);
            } else {
                unsigned char reply[length + 8];
                REP_GETPERM_ERR(reply) = 8;
                memcpy(REP_GETPERM_PERM(reply), permissions, length);
                free(permissions);
                z_reply(peer, reply, length + 8, message->id);
            }
            break;
        }

        case REQ_SETPERM: {
            CHECK_PERMISSION(REQ_SETPERM_PATH(message->content), ACL_ADMINISTRATE)
            unsigned char reply[REP_SETPERM_LENGTH];
            REP_SETPERM_ERR(reply) = set_permission(REQ_SETPERM_PATH(message->content), REQ_SETPERM_PK(message->content), REQ_SETPERM_PERM(message->content));
            z_reply(peer, reply, REP_SETPERM_LENGTH, message->id);
            break;
        }

        case REQ_TRUNCATE: {
            int res;
            unsigned char reply[REP_TRUNCATE_LENGTH];
            if (REQ_TRUNCATE_FH(message->content) == 0) {
                CHECK_PERMISSION(REQ_TRUNCATE_PATH(message->content), ACL_WRITE)
                res = truncate(REQ_TRUNCATE_PATH(message->content), REQ_TRUNCATE_LENGTH(message->content));
            } else {
                res = ftruncate(REQ_TRUNCATE_FH(message->content), REQ_TRUNCATE_LENGTH(message->content));
            }
            REP_TRUNCATE_ERR(reply) = res == -1 ? errno : 0;
            z_reply(peer, reply, REP_TRUNCATE_LENGTH, message->id);
            break;
        }

        default: {
            int64_t reply = -ENOSYS;
            z_reply(peer, (unsigned char *)reply, sizeof(int64_t), message->id);
            break;
        }
    }

    z_cleanup_message(message);
}


int host(char *ip, int port, unsigned char pk[ED25519_PK_LENGTH], unsigned char sk[ED25519_SK_LENGTH]) {
    char pk_bs32[PK_BS64_LENGTH];
    LocalPeer server;
    z_helpers_bin_to_bs32(pk, ED25519_PK_LENGTH, pk_bs32, PK_BS32_LENGTH);
    z_initialize_local_peer(&server, pk, sk, message_listener, connection_listener, disconnection_listener);
    printf("The server (%s) will listen on %s:%d ...\n", pk_bs32, ip, port);
    return z_listen(&server, ip, port);
}

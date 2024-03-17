#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/unistd.h>

#include "zrfs.h"
#include "protocol.h"

#define TIMEOUT 3

#ifdef __CYGWIN__
    #define __CYGWIN__ 0
#endif


void on_connection(RemotePeer *peer) {
    (void) peer;
    ConnectArgs *args = (ConnectArgs *)peer->attribute;
    pthread_mutex_lock(&args->mutex);
    pthread_cond_signal(&((ConnectArgs *)peer->attribute)->cond);
    pthread_mutex_unlock(&args->mutex);
}


void on_message(RemotePeer *peer, Message *message) {
    (void) peer;
    z_cleanup_message(message);
}


void on_disconnection(RemotePeer *peer) {
    (void) peer;
}


void *connect_d(void *arg) {
    ConnectArgs *args = (ConnectArgs *)arg;
    args->server->attribute = args;
    z_connect(&args->peer, args->server, args->host, args->port, args->server_pk);
    args->disconnection_listener(args->server);
    free(args);
    return NULL;
}


int quick_connect(RemotePeer *server,
                  const unsigned char pk[ED25519_PK_LENGTH],
                  const unsigned char sk[ED25519_SK_LENGTH],
                  const unsigned char server_pk[ED25519_PK_LENGTH],
                  char *host, int port,
                  DisconnectionListener disconnection_listener) {
    ConnectArgs *args = malloc(sizeof(ConnectArgs));
    pthread_t thread;

    z_initialize_local_peer(&args->peer, pk, sk, on_message, on_connection, on_disconnection);
    args->server_pk = server_pk;
    args->host = host;
    args->port = port;
    args->server = server;
    args->disconnection_listener = disconnection_listener;
    pthread_cond_init(&args->cond, NULL);
    pthread_mutex_init(&args->mutex, NULL);

    pthread_mutex_lock(&args->mutex);
    pthread_create(&thread, NULL, connect_d, args);
    pthread_detach(thread);
    pthread_cond_wait(&args->cond, &args->mutex);
    pthread_mutex_unlock(&args->mutex);
    return 0;
}


int z_fstat(RemotePeer *server, uint64_t fh, struct stat *statbuf) {
    Message *reply = NULL;
    unsigned char request[10];
    request[0] = REQ_GETATTR;
    REQ_GETATTR_FH(request) = fh;
    REQ_GETATTR_PATH(request)[0] = '\0';
    z_send(server, request, 10, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    if (REP_GETATTR_ERR(reply->content) != 0) {
        int res = -(int)REP_GETATTR_ERR(reply->content);
        z_cleanup_message(reply);
        return res;
    }

    statbuf->st_dev = REP_GETATTR_DEV(reply->content);
    statbuf->st_ino = REP_GETATTR_INO(reply->content);
    statbuf->st_size = REP_GETATTR_SIZE(reply->content);
    statbuf->st_blksize = REP_GETATTR_BLKSIZE(reply->content);
    statbuf->st_blocks = REP_GETATTR_BLOCKS(reply->content);
    statbuf->st_mode = REP_GETATTR_MODE(reply->content);
    #if __CYGWIN__
    statbuf->st_uid = -1;
    statbuf->st_gid = -1;
    #endif
    z_cleanup_message(reply);
    return 0;
}


int z_lstat(RemotePeer *server, const char *pathname, struct stat *statbuf) {
    Message *reply = NULL;
    int request_length = 10 + strlen(pathname);
    unsigned char request[request_length];
    request[0] = REQ_GETATTR;
    REQ_GETATTR_FH(request) = 0;
    strcpy(REQ_GETATTR_PATH(request), pathname);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    if (REP_GETATTR_ERR(reply->content) != 0) {
        int res = -(int)REP_GETATTR_ERR(reply->content);
        z_cleanup_message(reply);
        return res;
    }

    statbuf->st_dev = REP_GETATTR_DEV(reply->content);
    statbuf->st_ino = REP_GETATTR_INO(reply->content);
    statbuf->st_size = REP_GETATTR_SIZE(reply->content);
    statbuf->st_blksize = REP_GETATTR_BLKSIZE(reply->content);
    statbuf->st_blocks = REP_GETATTR_BLOCKS(reply->content);
    statbuf->st_mode = REP_GETATTR_MODE(reply->content);
    #if __CYGWIN__
    statbuf->st_uid = -1;
    statbuf->st_gid = -1;
    #endif
    z_cleanup_message(reply);
    return 0;
}


int z_readdir(RemotePeer *server, z_dirent **entries, int *entries_length, const char *path) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 2;
    unsigned char request[request_length];
    request[0] = REQ_READDIR;
    strcpy(REQ_READDIR_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    if (REP_READDIR_ERR(reply->content) != 0) {
        int res = -(int)REP_READDIR_ERR(reply->content);
        z_cleanup_message(reply);
        return res;
    }

    *entries = NULL;
    *entries_length = 0;
    unsigned char *reply_entry = reply->content + 8;
    while (reply_entry+50 < reply->content + reply->content_length) {
        (*entries_length)++;
        *entries = realloc(*entries, *entries_length * sizeof(z_dirent));
        z_dirent *entry = *entries + (*entries_length - 1);

        memset(&entry->st, 0, sizeof(struct stat));
        strcpy(entry->d_name, REP_ENTRY_NAME(reply_entry));
        entry->fill_dir_plus = REP_ENTRY_FDP(reply_entry);
        if (entry->fill_dir_plus) {
            entry->st.st_dev = REP_ENTRY_DEV(reply_entry);
            entry->st.st_ino = REP_ENTRY_INO(reply_entry);
            entry->st.st_size = REP_ENTRY_SIZE(reply_entry);
            entry->st.st_blksize = REP_ENTRY_BLKSIZE(reply_entry);
            entry->st.st_blocks = REP_ENTRY_BLOCKS(reply_entry);
            entry->st.st_mode = REP_ENTRY_MODE(reply_entry);
            #if __CYGWIN__
            entry->st.st_uid = -1;
            entry->st.st_gid = -1;
            #endif
        } else {
            entry->st.st_ino = REP_ENTRY_INO(reply_entry);
            entry->st.st_mode = REP_ENTRY_MODE(reply_entry);
        }
        reply_entry += 50 + strlen(entry->d_name);   // 1 (fdp) + 6*8 + 1 '\0'
    }
    z_cleanup_message(reply);
    return 0;
}


int z_mkdir(RemotePeer *server, const char *path) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 2;
    unsigned char request[request_length];
    request[0] = REQ_MKDIR;
    strcpy(REQ_MKDIR_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;

    int res = -(int)REP_MKDIR_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_rename(RemotePeer *server, const char *oldpath, const char *newpath) {
    Message *reply = NULL;
    const size_t request_length = strlen(oldpath) + strlen(newpath) + 3;
    unsigned char request[request_length];
    request[0] = REQ_RENAME;
    strcpy(REQ_RENAME_FROM(request), oldpath);
    strcpy(REQ_RENAME_TO(request), newpath);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = -(int)REP_RENAME_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_unlink(RemotePeer *server, const char *path) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 2;
    unsigned char request[request_length];
    request[0] = REQ_UNLINK;
    strcpy(REQ_UNLINK_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = -(int)REP_UNLINK_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_rmdir(RemotePeer *server, const char *path) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 2;
    unsigned char request[request_length];
    request[0] = REQ_RMDIR;
    strcpy(REQ_RMDIR_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = -(int)REP_RMDIR_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_create(RemotePeer *server, const char *path, int flag) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 10;
    unsigned char request[request_length];
    request[0] = REQ_CREATE;
    REQ_CREATE_FLAG(request) = flag;
    strcpy(REQ_CREATE_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = REP_CREATE_ERR(reply->content) != 0 ?
              -(int)REP_CREATE_ERR(reply->content) :
              REP_CREATE_FH(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_open(RemotePeer *server, const char *path, int flag) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 10;
    unsigned char request[request_length];
    request[0] = REQ_OPEN;
    REQ_OPEN_FLAG(request) = flag;
    strcpy(REQ_OPEN_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = REP_OPEN_ERR(reply->content) != 0 ?
            -(int)REP_OPEN_ERR(reply->content) :
            REP_OPEN_FH(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_pread(RemotePeer *server, int fd, void *buf, size_t nbytes, off_t offset) {
    Message *reply = NULL;
    unsigned char request[REQ_READ_LENGTH];
    request[0] = REQ_READ;
    REQ_READ_FH(request) = fd;
    REQ_READ_SIZE(request) = nbytes;
    REQ_READ_OFFSET(request) = offset;
    z_send(server, request, REQ_READ_LENGTH, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    if (REP_READ_ERR(reply->content) != 0) {
        int res = -(int)REP_READ_ERR(reply->content);
        z_cleanup_message(reply);
        return res;
    }

    unsigned int size = reply->content_length - 8;
    memcpy(buf, REP_READ_BUF(reply->content), size);
    z_cleanup_message(reply);
    return size;
}


int z_pwrite(RemotePeer *server, int fd, const void *buf, size_t nbytes, off_t offset) {
    Message *reply = NULL;
    const size_t request_length = 25 + nbytes;
    unsigned char request[request_length];
    request[0] = REQ_WRITE;
    REQ_WRITE_FH(request) = fd;
    REQ_WRITE_SIZE(request) = nbytes;
    REQ_WRITE_OFFSET(request) = offset;
    memcpy(REQ_WRITE_BUF(request), buf, nbytes);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = (int)REP_WRITE_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_statvfs(RemotePeer *server, const char *path, struct statvfs *stat) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 2;
    unsigned char request[request_length];
    request[0] = REQ_STATFS;
    strcpy(REQ_STATFS_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    const int err = -(int)REP_STATFS_ERR(reply->content);
    if (err == 0) {
        stat->f_bsize = REP_STATFS_BSIZE(reply->content);
        stat->f_frsize = REP_STATFS_FRSIZE(reply->content);
        stat->f_blocks = REP_STATFS_BLOCKS(reply->content);
        stat->f_bfree = REP_STATFS_BFREE(reply->content);
        stat->f_bavail = REP_STATFS_BAVAIL(reply->content);
        stat->f_files = REP_STATFS_FILES(reply->content);
        stat->f_ffree = REP_STATFS_FFREE(reply->content);
        stat->f_favail = REP_STATFS_FAVAIL(reply->content);
        stat->f_fsid = REP_STATFS_FSID(reply->content);
        stat->f_flag = REP_STATFS_FLAG(reply->content);
        stat->f_namemax = REP_STATFS_NAMEMAX(reply->content);
    }
    z_cleanup_message(reply);
    return err;
}


int z_release(RemotePeer *server, int fd) {
    Message *reply = NULL;
    unsigned char request[REQ_RELEASE_LENGTH];
    request[0] = REQ_RELEASE;
    REQ_RELEASE_FH(request) = fd;
    z_send(server, request, REQ_RELEASE_LENGTH, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = -(int)REP_RELEASE_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_getperm(RemotePeer *server, const char *path, size_t *length, char *permissions) {
    Message *reply = NULL;
    const size_t request_length = strlen(path) + 1;
    unsigned char request[request_length];
    request[0] = REQ_GETPERM;
    strcpy(REQ_GETPERM_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    if (REP_GETPERM_ERR(reply->content) != 0) {
        int res = -(int)REP_GETPERM_ERR(reply->content);
        z_cleanup_message(reply);
        return res;
    }
    *length = reply->content_length - 8;
    permissions = malloc(*length);
    memcpy(permissions, reply->content, *length);
    z_cleanup_message(reply);
    return 0;
}


int z_setperm(RemotePeer *server, const char *path, const char pk_bs64[PK_BS64_LENGTH], char permission) {
    Message *reply = NULL;
    const size_t request_length = 10 + strlen(path) + PK_BS64_LENGTH;
    unsigned char request[request_length];
    request[0] = REQ_SETPERM;
    REQ_SETPERM_PERM(request) = permission;
    memcpy(REQ_SETPERM_PK(request), pk_bs64, PK_BS64_LENGTH);
    strcpy(REQ_SETPERM_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = (int)REP_SETPERM_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_truncate(RemotePeer *server, const char *path, off_t length) {
    Message *reply = NULL;
    const size_t request_length = 18 + strlen(path);
    unsigned char request[request_length];
    request[0] = REQ_TRUNCATE;
    REQ_TRUNCATE_FH(request) = 0;
    REQ_TRUNCATE_LENGTH(request) = length;
    strcpy(REQ_TRUNCATE_PATH(request), path);
    z_send(server, request, request_length, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = -(int)REP_TRUNCATE_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}


int z_ftruncate(RemotePeer *server, int fd, off_t length) {
    Message *reply = NULL;
    unsigned char request[18];
    request[0] = REQ_TRUNCATE;
    REQ_TRUNCATE_FH(request) = fd;
    REQ_TRUNCATE_LENGTH(request) = length;
    REQ_TRUNCATE_PATH(request)[0] = '\0';
    z_send(server, request, 18, TIMEOUT, &reply);

    if (reply == NULL)
        return -ETIMEDOUT;
    int res = -(int)REP_TRUNCATE_ERR(reply->content);
    z_cleanup_message(reply);
    return res;
}

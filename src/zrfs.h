#ifndef ZRFS_LIBRARY_H
#define ZRFS_LIBRARY_H

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <zprotocol.h>


typedef struct ConnectArgs {
    const unsigned char *server_pk;
    char *host;
    int port;
    LocalPeer peer;
    RemotePeer *server;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    DisconnectionListener disconnection_listener;
} ConnectArgs;


/**
 * Connect to a ZRFS server.
 * @param server A pointer to an allocated space for the future RemotePeer.
 * @param pk The local peer public key.
 * @param sk The local peer secret key.
 * @param server_pk The server public key.
 * @param host The server host.
 * @param port The server port.
 * @param disconnection_listener A function called if the connection is stopped or loss.
 * @return 0 on success.
 */
int quick_connect(RemotePeer *server,
                  const unsigned char pk[ED25519_PK_LENGTH],
                  const unsigned char sk[ED25519_SK_LENGTH],
                  const unsigned char server_pk[ED25519_PK_LENGTH],
                  char *host, int port,
                  DisconnectionListener disconnection_listener);


typedef struct z_dirent {
    char d_name[NAME_MAX + 1];
    char fill_dir_plus;
    struct stat st;
} z_dirent;


int z_fstat(RemotePeer *server, uint64_t fh, struct stat *statbuf);
int z_lstat(RemotePeer *server, const char *pathname, struct stat *statbuf);
int z_readdir(RemotePeer *server, z_dirent **entries, int *entries_length, const char *fh);
int z_mkdir(RemotePeer *server, const char *path);
int z_rename(RemotePeer *server, const char *oldpath, const char *newpath);
int z_unlink(RemotePeer *server, const char *path);
int z_rmdir(RemotePeer *server, const char *path);
int z_create(RemotePeer *server, const char *path, int flag);
int z_open(RemotePeer *server, const char *path, int flag);
int z_pread(RemotePeer *server, int fd, void *buf, size_t nbytes, off_t offset);
int z_statvfs(RemotePeer *server, const char *path, struct statvfs *stat);
int z_release(RemotePeer *server, int fd);
int z_pwrite(RemotePeer *server, int fd, const void *buf, size_t nbytes, off_t offset);
int z_getperm(RemotePeer *server, const char *path, size_t *length, char *permissions);
int z_setperm(RemotePeer *server, const char *path, const char pk_bs64[PK_BS64_LENGTH], char permission);
int z_truncate(RemotePeer *server, const char *path, off_t length);
int z_ftruncate(RemotePeer *server, int fd, off_t length);

#endif
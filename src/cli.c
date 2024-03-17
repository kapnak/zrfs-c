#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h>

#include "server.h"
#include "pt.h"
#include "acl.h"


int usage(char *exe) {
    printf("Usage:\n"
           "\t%1$s host <ip> <port> [server sk path]\n"
           "\t%1$s mount <ip> <port> <z_mount point> <server pk bs32 / 0 (unencrypted)> [client sk path]\n"
           "\t%1$s acl add <file> <pk> <permission>\n"
           "\t%1$s acl del <file> [pk]\n"
           "Generate sk if the file doesn't exists and show the pk in base 32 :\n"
           "\t%1$s key <sk path>\n", exe);
    exit(1);
}


int main(int argc, char **argv) {
    if (argc < 2)
        usage(argv[0]);

    if (strcmp(argv[1], "host") == 0) {
        unsigned char pk[ED25519_PK_LENGTH];
        unsigned char sk[ED25519_SK_LENGTH];
        int port = atoi(argv[3]);
        if (argc == 4)
            z_helpers_generate_kp(pk, sk);
        else if (argc == 5)
            z_helpers_read_kp(argv[4], pk, sk);
        return host(argv[2], port, pk, sk);
    }
    else if (strcmp(argv[1], "mount") == 0) {
        unsigned char pk[ED25519_PK_LENGTH];
        unsigned char sk[ED25519_SK_LENGTH];
        unsigned char server_pk[ED25519_PK_LENGTH];
        if (argc == 6)
            z_helpers_generate_kp(pk, sk);
        else if (argc == 7)
            z_helpers_read_kp(argv[6], pk, sk);
        else
            usage(argv[0]);
        if (strcmp(argv[5], "0") == 0) {    // TODO : Avoid the repetition.
            return z_mount(argv[2], atoi(argv[3]), argv[4], pk, sk, NULL);
        } else {
            z_helpers_bs32_to_bin(argv[5], server_pk, ED25519_PK_LENGTH);
            return z_mount(argv[2], atoi(argv[3]), argv[4], pk, sk, server_pk);
        }
    }
    else if (strcmp(argv[1], "acl") == 0) {
        if (strcmp(argv[2], "add") == 0) {
            if (argc != 6)
                usage(argv[0]);

            char pk[PK_BS32_LENGTH];
            if (strcmp(argv[4], "0") == 0)
                memcpy(pk, ACL_DEFAULT_PK, PK_BS32_LENGTH);
            else if (strlen(argv[4]) != PK_BS32_LENGTH-1) {
                fprintf(stderr, "The public need to be %d char length.\n", PK_BS32_LENGTH-1);
                return 1;
            } else {
                memcpy(pk, argv[4], PK_BS32_LENGTH);
            }

            if (set_permission(argv[3], pk, atoi(argv[5])) != 0) {
                perror("failed");
                return -errno;
            }
        } else if (strcmp(argv[2], "del") == 0) {
            if (argc == 4)                      // If not pk given then delete all acl.
                return del_permissions(argv[3]);
            else if (strcmp(argv[4], "0") == 0) // If pk is 0 then use default null pk.
                return del_permission(argv[3], ACL_DEFAULT_PK);
            else
                return del_permission(argv[3], argv[5]);
        } else if (strcmp(argv[2], "list") == 0) {
            if (argc != 4)
                usage(argv[0]);

            ssize_t size = listxattr(argv[3], NULL, 0);
            if (size < 0)
                return -errno;
            char xattrs[size];
            if (listxattr(argv[3], xattrs, size) < 0)
                return -errno;
            char *xattr = xattrs;
            while (xattr < (xattrs + size)) {
                char permission;
                getxattr(argv[3], xattr, &permission, sizeof(char));
                printf("%s\t%d\n", xattr, permission);
                xattr += strlen(xattr) + 1;
            }
        } else {
            usage(argv[0]);
        }
    }
    else if (strcmp(argv[1], "key") == 0) {
        if (argc < 3)
            usage(argv[0]);
        unsigned char pk[ED25519_PK_LENGTH];
        unsigned char sk[ED25519_SK_LENGTH];
        char pk_bs32[PK_BS32_LENGTH];
        z_helpers_read_kp(argv[2], pk, sk);
        z_helpers_bin_to_bs32(pk, ED25519_PK_LENGTH, pk_bs32, PK_BS32_LENGTH);
        printf("%s\000\n", pk_bs32);
    }
    else {
        usage(argv[0]);
    }

    return 0;
}

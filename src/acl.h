#ifndef ACL_H
#define ACL_H

#include <zprotocol.h>

#define ACL_NOTHING      0
#define ACL_REFERENCE    1
#define ACL_READ         2
#define ACL_WRITE        3
#define ACL_ADMINISTRATE 4

#define ACL_PREFIX_LENGTH 11
#define ACL_PREFIX "user.z.acl."
#define ACL_DEFAULT_PK "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define ACL_DEFAULT "user.z.acl.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"


/**
 * Get permission of every user set on the file 'path'.
 * @param path The path of the file.
 * @param length The length of the permissions list will be set in it.
 * @return The permissions (dynamically allocated) concatenated in the format '[pk_bs_32<PK_BS32_LENGTH>][permission<char>]'.
 */
char *get_permissions(const char *path, size_t *length);

/**
 * Get permission of user identified by 'pk_bs32' on file 'path'.
 * @param path The path of the file.
 * @param pk_bs32 The user public key encoded in base 32.
 * @return A char number corresponding to the permission (cf. PERMISSION_*).
 */
char get_permission(const char *path, const char pk_bs32[PK_BS32_LENGTH]);

/**
 * Set the permission of the user identified by 'pk_bs32' on file 'path'.
 * @param path The path of the file.
 * @param pk_bs32 The user public key encoded in base 32.
 * @param permission The permission to set (cf. PERMISSION_*).
 * @return 0 on success else -errno.
 */
int set_permission(const char *path, char pk_bs32[PK_BS32_LENGTH], char permission);

/**
 * Delete all permissions of the file 'path'.
 * @param path The file.
 * @return 0 on success else errno.
 */
int del_permissions(const char *path);

/**
 * Delete permission of user idenified by 'pk_bs32' on file 'path'.
 * @param path The file.
 * @param pk_bs32 The user public key encoded in base 32.
 * @return
 */
int del_permission(const char *path, const char pk_bs32[PK_BS32_LENGTH]);

#endif
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/xattr.h>

#include "acl.h"


char *get_permissions(const char *path, size_t *length) {
    char *permissions = NULL;
    *length = 0;
    ssize_t size = listxattr(path, NULL, 0);
    if (size < 0)
        return NULL;

    // Get the attributes keys.
    char xattrs[size];
    ssize_t res = listxattr(path, xattrs, size);
    if (res < 0)
        return NULL;

    char *xattr = xattrs;
    while (xattr < (xattrs + size)) {
        if (strncmp(xattr, ACL_PREFIX, ACL_PREFIX_LENGTH) == 0 && strlen(xattr) == PK_BS32_LENGTH-1) {
            permissions = realloc(permissions, *length + PK_BS32_LENGTH);
            memcpy(permissions + *length, xattr, PK_BS32_LENGTH);
            getxattr(path, xattr, (permissions + *length + PK_BS32_LENGTH), sizeof(char));
            *length += PK_BS32_LENGTH + 1;
        }
        xattr += strlen(xattr) + 1;
    }
    return permissions;
}


char get_permission(const char *path, const char pk_bs32[PK_BS32_LENGTH]) {
    char acl_key[ACL_PREFIX_LENGTH + PK_BS32_LENGTH] = {ACL_PREFIX};
    memcpy(acl_key + ACL_PREFIX_LENGTH, pk_bs32, PK_BS32_LENGTH);

    // Get the total size of attribute keys.
    ssize_t size = listxattr(path, NULL, 0);
    if (size > 0) {
        // Get the attributes keys.
        char xattrs[size];
        if (listxattr(path, xattrs, size) < 0)
            return -errno;

        // Check if the pk_bs32 is in the attribute keys.
        char *xattr = xattrs;
        char *end = xattrs + size;
        char def = 0;
        while (xattr < end) {
            if (strcmp(xattr, ACL_DEFAULT) == 0)            // NULL_XATTR_KEY found.
                def = 1;
            if (strcmp(xattr, acl_key) == 0) {     // Found.
                char permission;
                getxattr(path, xattr, (void *)&permission, sizeof(char));
                return permission;
            }
            xattr += strlen(xattr) + 1;
        }

        if (def) {      // If NULL_XATTR_KEY found ; use it.
            char permission;
            getxattr(path, ACL_DEFAULT, (void *)&permission, sizeof(char));
            return permission;
        }
    }
    // Return if no parent directory.
    if (path[0] == '\0')
        return ACL_NOTHING;
    if ((path[0] == '/' || path[0] == '.') && path[1] == '\0')
        return ACL_NOTHING;
    if (strchr(path, '/') == NULL && strchr(path, '\\') == NULL)
        return ACL_NOTHING;

    // Get the parent directory permission and return it.
    char *pathdup = strdup(path);
    char *parent = dirname(pathdup);
    char permission = get_permission(parent, pk_bs32);
    free(pathdup);
    return permission;
}


int set_permission(const char *path, char pk_bs32[PK_BS32_LENGTH], char permission) {
    char acl_key[ACL_PREFIX_LENGTH + PK_BS32_LENGTH] = {ACL_PREFIX};
    memcpy(acl_key + ACL_PREFIX_LENGTH, pk_bs32, PK_BS32_LENGTH);
    if (setxattr(path, acl_key, (void *)&permission, sizeof(char), 0) == -1)
        return -errno;
    return 0;
}


int del_permissions(const char *path) {
    // Get the total size of attribute keys.
    ssize_t size = listxattr(path, NULL, 0);
    if (size > 0) {
        // Get the attributes keys.
        char xattrs[size];
        if (listxattr(path, xattrs, size) < 0)
            return -errno;

        char *xattr = xattrs;
        char *end = xattrs + size;
        char def = 0;
        while (xattr < end) {
            if (strncmp(xattr, ACL_PREFIX, ACL_PREFIX_LENGTH) == 0)
                removexattr(path, xattr);
            xattr += strlen(xattr) + 1;
        }
    }
    return 0;
}


int del_permission(const char *path, const char pk_bs32[PK_BS32_LENGTH]) {
    // Build ACL key with the pk.
    char acl_key[ACL_PREFIX_LENGTH + PK_BS32_LENGTH] = {ACL_PREFIX};
    memcpy(acl_key + ACL_PREFIX_LENGTH, pk_bs32, PK_BS32_LENGTH);

    ssize_t size = listxattr(path, NULL, 0);
    if (size < 0)
        return -errno;

    char xattrs[size];
    ssize_t res = listxattr(path, xattrs, size);
    if (res < 0)
        return -errno;

    char *xattr = xattrs;
    char *end = xattrs + size;

    while (xattr < end) {
        if (strcmp(xattr, acl_key) == 0)
            removexattr(path, xattr);
        xattr += strlen(xattr) + 1;
    }
    return 0;
}

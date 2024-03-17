#ifndef PT_H
#define PT_H

#include "zprotocol.h"


int z_mount(char *ip, int port,
            char *path,
            unsigned char pk[ED25519_PK_LENGTH],
            unsigned char sk[ED25519_SK_LENGTH],
            unsigned char server_pk[ED25519_PK_LENGTH]);

#endif
#ifndef SERVER_H
#define SERVER_H

#include <zprotocol.h>


int host(char *ip, int port, unsigned char pk[ED25519_PK_LENGTH], unsigned char sk[ED25519_SK_LENGTH]);

#endif
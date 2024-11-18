#ifndef CLIENT_H
#define CLIENT_H

#include "lobby.h"
#include "network.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
}Client;

#endif /* guard */

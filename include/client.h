#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include "network.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   bool in_game;
}Client;

#endif /* guard */

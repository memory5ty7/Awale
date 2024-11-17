#ifndef CLIENT_SERV_H
#define CLIENT_SERV_H

#include "server.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
}Client;

#endif /* guard */

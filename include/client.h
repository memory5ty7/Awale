#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdbool.h>

#include "network.h"
#include "constants.h"

#define BUF_SIZE    1024

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   bool in_game;
   bool logged_in;
   bool confirm_quit;
   bool in_queue;
   bool in_replay;
   char challenger[BUF_SIZE];
}Client;

#endif /* guard */


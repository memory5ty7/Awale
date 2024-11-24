#ifndef REPLAY_SESSION_H
#define REPLAY_SESSION_H

#include "game.h"
#include "client.h"

#define MAX_MESSAGE_LENGTH 256
#define CHAT_BUFFER_SIZE 10

typedef struct {
    gameState game;
    Client* player;
    char fileName[8];
    int* moves[BUF_SIZE];
    int turnNb;
} ReplaySession;

#endif /* guard */
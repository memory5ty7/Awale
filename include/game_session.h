#ifndef GAME_SESSION_H
#define GAME_SESSION_H

#include "game.h"
#include "client.h"

#define MAX_MESSAGE_LENGTH 256
#define CHAT_BUFFER_SIZE 10

typedef struct {
    gameState game;
    Client* players[2];
    Client* spectators[6];
    bool active;
    int nb_spectators;
    char fileName[8];
    FILE* file;
} GameSession;

#endif /* guard */
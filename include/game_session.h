#include "game.h"

#define MAX_SESSIONS 10
#define MAX_MESSAGE_LENGTH 256
#define CHAT_BUFFER_SIZE 10

typedef struct {
    gameState game;
    Client players[2];
    Client spectators[6];
    bool active;
    int nb_spectators;
    char fileName[8];
} GameSession;

GameSession sessions[MAX_SESSIONS];  // Toutes les sessions de jeu
int session_count = 0;
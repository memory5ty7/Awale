#include "game.h"

#define MAX_SESSIONS 10
#define MAX_MESSAGE_LENGTH 256
#define CHAT_BUFFER_SIZE 10

typedef struct {
    Client players[2];
    gameState game;
    bool active;
    Client spectators[6];
    char fileName[8];
} GameSession;

GameSession sessions[MAX_SESSIONS];  // Toutes les sessions de jeu
int session_count = 0;
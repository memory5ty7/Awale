#include "game.h"

#define MAX_SESSIONS 10
#define MAX_MESSAGE_LENGTH 256
#define CHAT_BUFFER_SIZE 10

typedef struct {
    char messages[CHAT_BUFFER_SIZE][MAX_MESSAGE_LENGTH];
    int messageCount;
} ChatBuffer;

typedef struct {
    Client players[2];
    gameState game;
    bool active;
    Client spectators[6];
    ChatBuffer chatBuffer;
} GameSession;

GameSession sessions[MAX_SESSIONS];  // Toutes les sessions de jeu
int session_count = 0;
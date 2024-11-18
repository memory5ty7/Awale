#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef struct gameState{
    int board[12];
    int stash[2];
    int rotation;
    bool movesAllowed[6];
    int current;
} gameState;

gameState initGame(int);
bool checkMove(gameState*, int);
bool checkWin(gameState);
void announceWinners(gameState);
void displayBoard(char*, size_t, gameState, int);
bool doMove(gameState*, int, int);

#endif // GAME_H
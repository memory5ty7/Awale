#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef struct gameState{
    int board[12];
    int stash[2];
    bool player1;
    int rotation;
    bool movesAllowed[6];
} gameState;

gameState initGame(int);
bool checkMove(gameState*);
void playerAction(gameState);
bool checkWin(gameState);
void announceWinners(gameState);
void displayBoard(char*, size_t, gameState, int);
bool doMove(gameState*, int);

#endif // GAME_H
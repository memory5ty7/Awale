#ifndef GAME_H
#define GAME_H

typedef struct gameState{
    int board[12];
    int stash[2];
    bool player1;
    int rotation;
} gameState;

gameState initGame(int);
bool checkMove(gameState);
void playerAction(gameState);
bool checkWin(gameState);
void announceWinners(gameState);

void displayBoard(int*, int*, bool);
bool doMove(int*, int*, int);

#endif // GAME_H
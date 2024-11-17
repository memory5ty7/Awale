#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "game.h"

gameState initGame(int rotation){

    gameState state;
    
    for(int i=0;i<sizeof(state.board)/sizeof(int);i++){
        state.board[i]=4;
    }
    for(int i=0;i<sizeof(state.stash)/sizeof(int);i++){
        state.stash[i]=0;
    }
    state.player1 = true;

    state.rotation = rotation;

    return state;
}

bool checkMove(gameState *state){

        int nbAllowed=0;

        gameState testState;
        for(int i=0;i<sizeof(testState.board)/sizeof(int);i++){
            testState.board[i]=state->board[i];
        }
        for(int i=0;i<sizeof(testState.stash)/sizeof(int);i++){
            testState.stash[i]=state->stash[i];
        }
        testState.player1=state->player1;
        testState.rotation=state->rotation;

        bool finished=false;

        for(int i=0;i<6;i++){
            for(int j=0;j<sizeof(testState.board)/sizeof(int);j++){
                testState.board[j]=state->board[j];
            }
            testState.stash[0]=0;
            testState.stash[1]=0;
            
            if(doMove(state,i+1)){
                state->movesAllowed[i]=true;
                nbAllowed++;
            } else {
                state->movesAllowed[i]=false;
            }
        }
        if(nbAllowed==0){
            state->stash[!state->player1]+=48-state->stash[0]-state->stash[1];
            finished=true;
        }
        return finished;
}

void playerAction(gameState state){
    int choice=-1;
    while(choice<1||choice>6){
        printf("Choisissez une case.\n");
        scanf("%d",&choice);
        if(state.movesAllowed[choice-1]){
            break;
        } else {
            choice=-1;
            printf("Cette action n'est pas possible.\n");
        }
    }
    doMove(&state,choice);
    state.player1=!state.player1;
}

bool checkWin(gameState state){
    if(state.stash[0]>24||state.stash[1]>24){
        return true;
    }
    return false;
}

void announceWinners(gameState state){
    int winner;
    if(state.stash[0]>state.stash[1]){
        winner=0;
    } else {
        winner=1;
    }
    printf("La partie est terminée!\nLe joueur %d a gagné!",winner+1);
}

void displayBoard(char *buffer, size_t bufferSize, gameState state,int player) {
    int offset = 0;

    offset += snprintf(buffer + offset, bufferSize - offset, 
                       "Joueur %d | Stash: %d\n|", !player ? 1 : 2, 
                       state.stash[!state.player1]);

    if (!player) {
        for (int i = 11; i > 5; i--) {
            offset += snprintf(buffer + offset, bufferSize - offset, "%02d|", state.board[i]);
        }
        offset += snprintf(buffer + offset, bufferSize - offset, "\n|");
        for (int i = 0; i < 6; i++) {
            offset += snprintf(buffer + offset, bufferSize - offset, "%02d|", state.board[i]);
        }
    } else {
        for (int i = 5; i >= 0; i--) {
            offset += snprintf(buffer + offset, bufferSize - offset, "%02d|", state.board[i]);
        }
        offset += snprintf(buffer + offset, bufferSize - offset, "\n|");
        for (int i = 6; i < 12; i++) {
            offset += snprintf(buffer + offset, bufferSize - offset, "%02d|", state.board[i]);
        }
    }
    offset += snprintf(buffer + offset, bufferSize - offset, "\n  ");

    for (int i = 0; i < 6; i++) {
        if (state.movesAllowed[i]) {
            offset += snprintf(buffer + offset, bufferSize - offset, "%d  ", i + 1);
        } else {
            offset += snprintf(buffer + offset, bufferSize - offset, "   ");
        }
    }
    snprintf(buffer + offset, bufferSize - offset, "\n");
}

bool doMove(gameState *state,int choice){
        int j,nbSeeds, shift, chosen, totalstash;
        if(state->player1){
            shift=0;
        } else {
            shift=6;
        }
        chosen=choice+shift-1;
        nbSeeds=state->board[chosen];
        if(nbSeeds==0){
            return false;
        }
        //printf("Vous avez choisi la case %d qui contient %d graines\n", choice, nbSeeds);
        state->board[chosen]=0;
        j=chosen+1;
        while(nbSeeds>0){
            if(j%12!=chosen){
                state->board[j%12]+=1;
                nbSeeds--;
            }
            j+=state->rotation;
        }
        //printf("j = %d\n",j);
        totalstash=0;
        while((state->board[(j-1)%12]==2||state->board[(j-1)%12]==3)&&j%12<12-shift&&j%12>=6-shift){
            state->stash[!state->player1]+=state->board[(j-1)%12];
            totalstash+=state->board[(j-1)%12];
            state->board[(j-1)%12]=0;
            j-=state->rotation;
        }

        for(int i=0;i<6;i++){
            if(state->board[i+shift]!=0) return true;
        }
        return false;
        //if (totalstash)
        //    printf("Player %d, %d added to stash\n",!player1+1, totalstash);
}
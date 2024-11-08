#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "game.h"

gameState initGame(int rotation){

    gameState state;
    
    for(int i=0;i<sizeof(board)/sizeof(int);i++){
        state.board[i]=4;
    }
    for(int i=0;i<sizeof(state.stash)/sizeof(int);i++){
        state.stash[i]=0;
    }
    state.player1 = true;

    state.rotation = rotation;

    return state;
}

bool checkMove(gameState state){

        int nbAllowed=0;
        int testBoard[12];
        int testStash[2];

        for(int i=0;i<6;i++){
            for(int i=0;i<sizeof(testBoard)/sizeof(int);i++){
                testBoard[i]=state.board[i];
            }
            testStash[0]=0;
            testStash[1]=0;
            if(doMove(testBoard,testStash,i+1)){
                movesAllowed[i]=true;
                nbAllowed++;
            } else {
                movesAllowed[i]=false;
            }
        }
        if(nbAllowed==0){
            state.stash[!player1]+=48-state.stash[0]-state.stash[1];
            finished=true;
            break;
        }

}

void playerAction(gameState state){
    choice=-1;
    while(choice<1||choice>6){
        displayBoard(state.board, state.stash, player1);
        printf("Choisissez une case.\n");
        scanf("%d",&choice);
        if(movesAllowed[choice-1]){
            break;
        } else {
            choice=-1;
            printf("Cette action n'est pas possible.\n");
        }
    }
    doMove(state.board,state.stash,choice);
    state.player1=!state.player1;
}

bool checkWin(gameState state){
    if(state.stash[0]>24||state.stash[1]>24){
        return true;
    }
    return false;
}

void announceWinners(gameState state){
    if(state.stash[0]>state.stash[1]){
        winner=0;
    } else {
        winner=1;
    }
    printf("La partie est terminée!\nLe joueur %d a gagné!",winner+1);
}

void displayBoard(int* board, int* stash, bool player1){
    //printf("|");
    if(player1){
        printf("Joueur 1 | Stash: %d\n|",stash[!player1]);
        for(int i=11;i>5;i--){
            printf("%02d|",board[i]);
        }
        printf("\n|");
        for(int i=0;i<6;i++){
            printf("%02d|",board[i]);
        }
        printf("\n");
    } else {
        printf("Joueur 2 | Stash: %d\n|",stash[!player1]);
        for(int i=5;i>=0;i--){
            printf("%02d|",board[i]);
        }
        printf("\n|");
        for(int i=6;i<12;i++){
            printf("%02d|",board[i]);
        } 
        printf("\n");
    }
    printf("  ");
    for(int i=0;i<6;i++){
        if(movesAllowed[i]){
            printf("%d  ",i+1);
        } else {
            printf("   ");
        }
    }
    printf("\n");
}

bool doMove(int* board, int* stash, int choice){
        int j,nbSeeds, shift, chosen, totalstash;
        if(player1){
            shift=0;
        } else {
            shift=6;          
        }
        chosen=choice+shift-1;
        nbSeeds=board[chosen];
        if(nbSeeds==0){
            return false;
        }
        //printf("Vous avez choisi la case %d qui contient %d graines\n", choice, nbSeeds);
        board[chosen]=0;
        j=chosen+1;
        while(nbSeeds>0){
            if(j%12!=chosen){
                board[j%12]+=1;
                nbSeeds--;
            }
            j+=state.rotation;
        }
        //printf("j = %d\n",j);
        totalstash=0;
        while((board[(j-1)%12]==2||board[(j-1)%12]==3)&&j%12<12-shift&&j%12>=6-shift){
            stash[!player1]+=board[(j-1)%12];
            totalstash+=board[(j-1)%12];
            board[(j-1)%12]=0;
            j-=state.rotation;
        }

        for(int i=0;i<6;i++){
            if(board[i+shift]!=0) return true;
        }
        return false;
        //if (totalstash)
        //    printf("Player %d, %d added to stash\n",!player1+1, totalstash);
}
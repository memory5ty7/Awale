#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include "../include/server_state.h"
#include "../include/io.h"
#include "../include/util.h"

void start_replay_session(ServerState *serverState, char *buffer, Client *player, char *filename, int *moves, int nbMoves, ReplaySession *session)
{
    if (serverState->rSession_count >= MAX_SESSIONS)
    {
        write_client(player->sock, "Le serveur est plein.\n");
        return;
    }

    serverState->rSession_count++;

    session->game = initGame(1);
    session->player = player;
    strcpy(session->fileName, filename);
    for (int j = 0; j < nbMoves; j++)
    {
        session->moves[j] = moves[j];
    }
    session->turnNb = 0;
    session->game.current = 0;

    displayBoard(buffer, BUF_SIZE, session->game, 3);
    write_client(session->player->sock, buffer);
}

void handle_replay_session(ServerState *serverState, char *buffer, int len_buf, ReplaySession *session)
{
    if (len_buf <= 0)
    {
        //session->active = false;

        closesocket(session->player->sock);
        remove_client(serverState, session->player);
        return;
    }

    if(session->moves[session->turnNb] == 0)
    {
        end_replay(buffer, session);
        return;
    }

    session->game.current = 1 - session->game.current;

    sprintf(buffer,"\nTour %d : Joueur %d : Case %d\n", session->turnNb+1,(session->turnNb)%2+1,session->moves[session->turnNb]);
    write_client(session->player->sock, buffer);

    doMove(&(session->game), session->moves[session->turnNb++], (session->turnNb)%2);

    

    displayBoard(buffer, BUF_SIZE, session->game, 3);
    write_client(session->player->sock, buffer);
}

void end_replay(char *buffer, ReplaySession *session)
{
    write_client(session->player->sock, "La partie est terminée!\n");

    session->player->in_replay = false;

    // session->active = false;
}
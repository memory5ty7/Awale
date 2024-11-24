#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include "client.h"
#include "game_session.h"
#include "replay_session.h"

typedef struct ServerState{
    Client clients[MAX_CLIENTS];
    int nb_clients;
    GameSession sessions[MAX_SESSIONS];
    int session_count;
    ReplaySession rSessions[MAX_SESSIONS];
    int rSession_count;
    Client* waiting_clients[2];
    int waiting_count;
    char userPwd[NB_USERS][NB_CHAR_PER_USERPWD];
    int nbUsers;
    //FILE* users;
} ServerState;

#endif /* guard */
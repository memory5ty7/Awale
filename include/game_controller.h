#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "server.h"
#include "game_session.h"
#include "client.h"

void start_game_session(ServerState*, char *, Client, Client, GameSession *);
void handle_game_session(ServerState, char *, int, GameSession *);
void spectator_join_session(char *, Client *, GameSession *);
void end_game(char *, GameSession *);

#endif /* guard */
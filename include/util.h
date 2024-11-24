#ifndef UTIL_H
#define UTIL_H

#include "server.h"
#include "game_session.h"

void sanitizeFilename(char *);
int getClientID(ServerState, char *);
GameSession *getSessionByClient(ServerState*, Client *);
GameSession *getReplayByClient(ServerState*, Client *);
bool isSpectator(Client *, GameSession *);
bool check_if_player_is_connected(ServerState, char *);
bool authentification(char *userpassword, ServerState serverState);

void clear_clients(Client *, int);
void remove_client(ServerState*, int);
int read_client(SOCKET, char *);
void write_client(SOCKET, const char *);
void send_message_to_all_clients(ServerState, Client, const char *, char);

int init_connection(void);
void end_connection(int sock);

#endif /* guard */
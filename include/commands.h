#ifndef COMMANDS_H
#define COMMANDS_H

#include "server.h"
#include "client.h"

void cmd_chat(ServerState*, Client*);
void cmd_game(ServerState*, Client*, const char*);
void cmd_help(Client*);
void cmd_join(ServerState*, Client*, const char*);
void cmd_login(ServerState*, Client*, char*);
void cmd_msg(ServerState*, Client*, const char*);
void cmd_quit(ServerState*, Client*, const char*);
void cmd_register(ServerState*, Client*, char*);
void cmd_replay(ServerState*, Client*, const char*);
void cmd_showgames(ServerState*, Client*, const char*);
void cmd_showusers(ServerState*, Client*, const char*);

#endif /* guard */
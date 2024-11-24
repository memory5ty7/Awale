#ifndef REPLAY_CONTROLLER_H
#define REPLAY_CONTROLLER_H

#include "server.h"
#include "replay_session.h"
#include "client.h"

void start_replay_session(ServerState*, char *, Client *, char *, int *, int, ReplaySession *);
void handle_replay_session(ServerState*, char *, int, ReplaySession *);
void end_replay(char *, ReplaySession *);

#endif /* guard */
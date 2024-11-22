#ifndef IO_H
#define IO_H

#include "server_state.h"

void updateScores(const char *, const char *, const char *);
bool loadUsers(char *, ServerState*);

#endif /* guard */
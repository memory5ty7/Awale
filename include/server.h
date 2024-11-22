#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#else

#error not defined for this platform

#endif

#include "network.h"
#include "constants.h"
#include "server_state.h"

#include "util.h"

static void init(void);
static void end(void);
static void app(void);

#endif /* guard */

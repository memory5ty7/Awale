#ifndef CLIENT_H
#define CLIENT_H

#ifdef WIN32

#include <winsock2.h>


#elif defined (linux)

#include "network.h"


#else

#error not defined for this platform

#endif

#define CRLF     "\r\n"
#define PORT     1977

#define BUF_SIZE 1024

static void init(void);
static void end(void);
static void app(const char *address, const char *name, const char* pwd);
static int init_connection(const char *address);
static void end_connection(int sock);
static int read_server(SOCKET sock, char *buffer);
static void write_server(SOCKET sock, const char *buffer);

#endif /* guard */

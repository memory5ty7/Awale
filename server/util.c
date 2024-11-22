#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include "../include/server_state.h"

void sanitizeFilename(char *filename)
{
   size_t len = strlen(filename);
   for (size_t i = 0; i < len; i++)
   {
      if (filename[i] == '\n' || filename[i] == '\r')
      {
         filename[i] = '\0'; // Replace newline or carriage return with null terminator
         break;
      }
   }
}

int getClientID(ServerState serverState, char *name)
{
   for (int i = 0; i < serverState.nb_clients; i++)
   {
      if (strcmp(name, serverState.clients[i].name) == 0)
      {
         return i;
      }
   }
   return -1;
}

GameSession *getSessionByClient(ServerState serverState, Client *client)
{
   for (int i = 0; i < serverState.session_count; i++)
   {
      if (serverState.sessions[i].players[0].sock == client->sock || serverState.sessions[i].players[1].sock == client->sock)
      {
         return &serverState.sessions[i];
      }
      for (int j = 0; j < serverState.sessions[i].nb_spectators; j++)
      {
         if (serverState.sessions[i].spectators[j].sock == client->sock)
         {
            return &serverState.sessions[i];
         }
      }
   }
   return NULL;
}

bool isSpectator(Client *client, GameSession *session)
{
   if (session != NULL)
   {
      for (int j = 0; j < session->nb_spectators; j++)
      {
         if (strcmp(session->spectators[j].name, client->name) == 0)
         {
            return true;
         }
      }
   }
   return false;
}

int check_if_player_is_connected(ServerState serverState, char *name)
{

   for (int i = 0; i < serverState.nb_clients; i++)
   {
      if (strcmp(serverState.clients[i].name, name) == 0 && serverState.clients[i].logged_in)
      {
         return 1; // Player found
      }
   }
   return 0; // Player not found
}

bool authentification(char *userpassword, ServerState serverState)
{
   for (int i = 0; i < serverState.nbUsers; i++)
   {
      if (strcmp(serverState.userPwd[i], userpassword) == 0)
      {
         return true;
      }
   }
   return false;
}

void remove_client(ServerState *serverState, int to_remove)
{
   /* we remove the client in the array */
   memmove(serverState->clients + to_remove, serverState->clients + to_remove + 1, (serverState->nb_clients - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   serverState->nb_clients--;
}

int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

void send_message_to_all_clients(ServerState serverState, Client sender, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < serverState.nb_clients; i++)
   {
      /* we don't send message to the sender nor to not logged in clients*/
      if (serverState.clients[i].logged_in && sender.sock != serverState.clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " (chat général) : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(serverState.clients[i].sock, message);
      }
   }
}

int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

void end_connection(int sock) { closesocket(sock); }

void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}
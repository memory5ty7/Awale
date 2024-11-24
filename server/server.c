#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include "../include/server_state.h"
#include "../include/util.h"

/*
A debbuger : quand un joueur disconnecte ça envoie le message "... disconnected" 1 fois au 1er client puis 2 fois au deuxième etc.

Florian :
- afficher les joueurs en ligne et pouvoir challenger qui on veut
+ ajouter une commande /quit pour quitter le serveur (pas brutalement)
+ pouvoir login et register sur le serv et pas en ligne de commande
+ persistance des users
+ chat in game (ajouter un statut in game aux clients puis gérer en fonction du statut vers ligne 420 et + ) --> /chat pour parler sinon considère que c'est le move
Amaury :
- replay des parties
+ rajouter un système de score ou ratio pour les joueurs
+ persistance des parties
+ spec des parties en cours (et participer au chat)
*/

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE]; // initilisation du buffer de lecture et d'écriture
   ServerState serverState[1];
   if (!loadUsers("users", serverState))
   {
      perror("loading users failed");
      exit(errno);
   }

   serverState->session_count = 0;

   serverState->waiting_count = 0;

   serverState->nb_clients = 0; // nb de clients connectés

   int max_fd = sock;
   fd_set rdfs;

   while (1)
   {
      FD_ZERO(&rdfs);
      FD_SET(sock, &rdfs);

      for (int i = 0; i < serverState->waiting_count; i++)
      {
         FD_SET(serverState->waiting_clients[i]->sock, &rdfs);
         if (serverState->waiting_clients[i]->sock > max_fd)
         {
            max_fd = serverState->waiting_clients[i]->sock;
         }
      }

      /* add socket of each client */
      for (int i = 0; i < serverState->nb_clients; i++)
      {
         FD_SET(serverState->clients[i].sock, &rdfs);
      }
      for (int i = 0; i < serverState->session_count; i++)
      {
         if (serverState->sessions[i].active)
         {
            for (int j = 0; j < 2; j++)
            {
               FD_SET(serverState->sessions[i].players[j]->sock, &rdfs);
               if (serverState->sessions[i].players[j]->sock > max_fd)
               {
                  max_fd = serverState->sessions[i].players[j]->sock;
               }
            }
         }
      }

      if (select(max_fd + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }
      if (FD_ISSET(sock, &rdfs))
      {
         SOCKADDR_IN csin = {0};
         size_t sinsize = sizeof(csin);
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         // check toutes les erreurs possibles
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }
         else if (read_client(csock, buffer) <= 0)
         {
            closesocket(csock);
            continue;
         }
         else if (serverState->nb_clients == MAX_CLIENTS)
         {
            write_client(csock, "Il n'y a plus de place sur le serveur.\n");
            closesocket(csock);
            continue;
         }
         else
         {
            /* what is the new maximum fd ? */
            max_fd = csock > max_fd ? csock : max_fd;

            Client new_client = {csock};
            FD_SET(csock, &rdfs);
            new_client.in_game = false;
            new_client.logged_in = false;
            new_client.in_queue = false;
            strcpy(new_client.challenger,"");
            strcpy(buffer, "\n\nBonjour. Bienvenue sur Awale! \nVous pouvez vous connecter avec /login [username] [password]\nSi c'est la première fois que vous vous connectez, utilisez /register [username] [password]\n\nSi vous souhaitez quitter le serveur tapez : /quit\n");
            write_client(csock, buffer);

            serverState->clients[serverState->nb_clients] = new_client;
            serverState->nb_clients++;
         }
      }
      else
      {
         for (int i = 0; i < serverState->nb_clients; i++)
         {
            /* a client is talking */
            if (FD_ISSET(serverState->clients[i].sock, &rdfs))
            {
               Client *client = &serverState->clients[i];
               int c = read_client(client->sock, buffer);
               /* logged_in player disconnected */
               if (c == 0 && client->logged_in && !client->in_game)
               {
                  closesocket(client->sock);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(*serverState, *client, buffer, 1);
                  remove_client(serverState, i);
                  // server trace
                  puts(buffer);
               }
               else if (c == 0 && !client->logged_in)
               { // client disconnected
                  closesocket(client->sock);
                  remove_client(serverState, i);
                  // server trace
                  puts("client disconnected");
               }
               else if (!client->logged_in) // le client doit se connecter ou s'inscrire avant de pouvoir faire des actions
               {
                  char bufferTemp[BUF_SIZE];

                  strncpy(bufferTemp, buffer, BUF_SIZE - 1);
                  bufferTemp[BUF_SIZE - 1] = '\0'; // S'assurer que bufferTemp est nul-terminé

                  char *cmd = strtok(buffer, " ");    // get command

                  if (strcmp(cmd, "/login") == 0)
                  {
                     cmd_login(serverState, client, bufferTemp);
                  }
                  else if (strcmp(cmd, "/register") == 0)
                  {
                     cmd_register(serverState, client, bufferTemp);
                  }
                  else if (strcmp(cmd, "/quit") == 0)
                  {
                     cmd_quit(serverState, client, bufferTemp);
                  }
                  else
                  {
                     strcpy(buffer, "\nVeuillez rentrer une commande valide.\nVous pouvez vous connecter avec /login [username] [password]\nSi c'est la première fois que vous vous connectez, utilisez /register [username] [password]\n\nSi vous souhaitez quitter le serveur tapez : /quit\n");
                     write_client(client->sock, buffer);
                  }
               }
               else // sinon le client est connecté et peut faire toutes les actions
               {
                  // input is a command
                  if (strncmp(buffer, "/", 1) == 0)
                  {
                     char bufferTemp[BUF_SIZE];
                     strncpy(bufferTemp, buffer, BUF_SIZE - 1);
                     bufferTemp[BUF_SIZE - 1] = '\0'; // S'assurer que bufferTemp est nul-terminé

                     char *cmd = strtok(buffer, " ");

                     if (client->in_game && strcmp(cmd, "/chat") == 0)
                     {
                        cmd_chat(serverState, client, bufferTemp);
                     }
                     else if (!client->in_game && !client->in_queue && strcmp(cmd, "/game") == 0)
                     {
                        cmd_game(serverState, client, bufferTemp);
                     }
                     else if (!client->in_game && !client->in_queue && strcmp(cmd, "/showgames") == 0)
                     {
                        cmd_showgames(client, bufferTemp);
                     }
                     else if (!client->in_game && !client->in_queue && strcmp(cmd, "/join") == 0)
                     {
                        cmd_join(serverState, client, bufferTemp);
                     }
                     else if (!client->in_game && !client->in_queue && strcmp(cmd, "/accept") == 0)
                     {
                        cmd_accept(serverState, client, bufferTemp);
                     }
                     else if (!client->in_game && !client->in_queue && strcmp(cmd, "/decline") == 0)
                     {
                        cmd_decline(serverState, client, bufferTemp);
                     }
                     else if (!client->in_game && !client->in_queue && strcmp(cmd, "/replay") == 0)
                     {
                        cmd_replay(serverState, client, bufferTemp);
                     }
                     else if (client->in_queue && strcmp(cmd, "/cancel") == 0)
                     {
                        cmd_cancel(serverState, client, bufferTemp);
                     }
                     else if (strcmp(cmd, "/msg") == 0)
                     {
                        cmd_msg(serverState, client, bufferTemp, i);
                     }
                     else if (strcmp(cmd, "/showusers") == 0)
                     {
                        cmd_showusers(serverState, client, bufferTemp);
                     }
                     else if (strcmp(cmd, "/help") == 0)
                     {
                        cmd_help(client, bufferTemp);
                     }
                     else if (strcmp(cmd, "/quit") == 0)
                     {
                        cmd_quit(serverState, client, bufferTemp);
                     }
                     else
                     {
                        write_client(client->sock, "Commande inconnue. Tapez /help pour voir les commandes disponibles.\n");
                     }
                  }
                  // si ce n'est pas une commande, soit le joueur est in game et fait un coup ou confirme son /quit soit il parle dans le chat lobby
                  else if (client->in_game)
                  {
                     if (client->confirm_quit)
                     {
                        if (strcmp(buffer, "y") == 0 || strcmp(buffer, "Y") == 0 || strcmp(buffer, "yes") == 0)
                        {
                           write_client(client->sock, "You have quit the game. You will be disconnected from the server.\n");
                           closesocket(client->sock);
                           strncpy(buffer, client->name, BUF_SIZE - 1);
                           strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                           send_message_to_all_clients(*serverState, *client, buffer, 1);
                           remove_client(serverState, i);
                           // server trace
                           puts(buffer);
                        }
                        else if (strcmp(buffer, "n") == 0 || strcmp(buffer, "N") == 0 || strcmp(buffer, "no") == 0)
                        {
                           write_client(client->sock, "/quit canceled.\nYou have decided to continue the game. \n");
                           client->confirm_quit = false;
                        }
                        else
                        {
                           write_client(client->sock, "Please enter y or n.\n");
                        }
                     }
                     else
                     {
                        GameSession *session = getSessionByClient(serverState, client);

                        if (session != NULL)
                        {
                           // Seul le joueur actif peut effectuer un move
                           if (strcmp(session->players[session->game.current]->name, client->name) == 0)
                              handle_game_session(serverState, buffer, c, session);
                           else
                              write_client(client->sock, "Ce n'est pas votre tour.\n/chat [message] pour parler\n");
                        }
                        else
                        {
                           puts("session not found for client :");
                           puts(client->name);
                        }
                     }
                  }
                  else
                  {
                     send_message_to_all_clients(*serverState, *client, buffer, 0);
                  }
                  break;
               }
            }
         }
      }
   }

   end_connection(sock);
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
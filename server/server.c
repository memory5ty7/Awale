#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include "../include/server.h"
#include "../include/game_session.h"

#define NB_USERS 10
#define NB_CHAR_PER_USERPWD 30
/*
Florian :
- afficher les joueurs en ligne et pouvoir challenger qui on veut
- ajouter une commande /quit pour quitter le serveur (pas brutalement)
- pouvoir login et register sur le serv et pas en ligne de commande
- chat in game (ajouter un statut in game aux clients puis gérer en fonction du statut vers ligne 420 et + ) --> /chat pour parler sinon considère que c'est le move
Amaury :
- replay des parties
- rajouter un système de score ou ratio pour les joueurs
- persistance des parties ou des users
- spec des parties en cours (et participer au chat)

*/

bool loadUsers(char *filename, char **userPwd, int *nbUsers)
{
   FILE *fptr = fopen(filename, "r");
   if (fptr == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return false;
   }

   for (int i = 0; i < NB_USERS; i++)
   {
      userPwd[i] = malloc(sizeof(char) * NB_CHAR_PER_USERPWD);
      if (!userPwd[i])
      {
         perror("malloc failed");
         fclose(fptr);
         return false;
      }
   }

   char line[NB_CHAR_PER_USERPWD];
   while (fgets(line, sizeof(line), fptr) && *nbUsers < NB_USERS)
   {
      line[strcspn(line, "\r\n")] = '\0'; // Retire le \n
      strcpy(userPwd[*nbUsers], line);
      (*nbUsers)++;
   }

   fclose(fptr);
   return true;
}

bool authentification(char *userpassword, char **userPwd, int nbUsers)
{
   for (int i = 0; i < nbUsers; i++)
   {
      if (strcmp(userPwd[i], userpassword) == 0)
      {
         return true;
      }
   }
   return false;
}

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

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static int init_connection(void)
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

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
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

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

void start_game_session(char *buffer, Client player1, Client player2, int session_count, GameSession *session)
{

   if (session_count >= MAX_SESSIONS)
   {
      write_client(player1.sock, "Le serveur est plein.\n");
      write_client(player2.sock, "Le serveur est plein.\n");
      closesocket(player1.sock);
      closesocket(player2.sock);
      return;
   }

   session->players[0] = player1;
   session->players[1] = player2;
   session->game = initGame(1);
   session->active = true;

   char filename[16];
   time_t current_time = time(NULL);
   struct tm *local_time = localtime(&current_time);
   int hours = local_time->tm_hour;
   int minutes = local_time->tm_min;
   int seconds = local_time->tm_sec;
   snprintf(filename, sizeof(filename), "%02d:%02d:%02d\n", hours, minutes);
   strcpy(session->fileName, filename);
   write_client(player1.sock, session->fileName);
   write_client(player2.sock, session->fileName);
   char msg[1024];

   snprintf(msg, sizeof(msg), "La partie contre %s commence !\n", player2.name);
   write_client(player1.sock, msg);

   snprintf(msg, sizeof(msg), "La partie contre %s commence !\n", player1.name);
   write_client(player2.sock, msg);

   checkMove(&(session->game), session->game.current);

   displayBoard(buffer, BUF_SIZE, session->game, 0);
   write_client(session->players[0].sock, buffer);

   displayBoard(buffer, BUF_SIZE, session->game, 1);
   write_client(session->players[1].sock, buffer);
}

void handle_game_session(char *buffer, int len_buf, GameSession *session)
{
   puts("handle game session");
   Client *players;
   Client *current;
   Client *opponent;

   players = session->players;
   current = &players[session->game.current];
   opponent = &players[1 - session->game.current];

   puts("move of current player : ");
   puts(buffer);
   if (len_buf <= 0)
   {
      snprintf(buffer, BUF_SIZE, "%s a quitté la partie. Fin de la partie.\n", current->name);
      write_client(opponent->sock, buffer);

      end_connection(current->sock);
      end_connection(opponent->sock);

      session->active = false;
      return;
   }
   buffer[len_buf] = '\0';

   int choice = atoi(buffer);

   if (choice < 1 || choice > 6 || !session->game.movesAllowed[choice - 1])
   {
      write_client(current->sock, "Invalid move 1. Try again.\n");
      return;
   }

   if (!doMove(&(session->game), choice, session->game.current))
   {
      write_client(current->sock, "Invalid move 2. Try again.\n");
      return;
   }

   if (checkWin(session->game))
   {
      end_game(buffer, session);
      return;
   }

   session->game.current = 1 - session->game.current;

   if (checkMove(&(session->game), session->game.current))
   {
      end_game(buffer, session);
      return;
   }

   displayBoard(buffer, BUF_SIZE, session->game, 0);
   write_client(session->players[0].sock, buffer);

   displayBoard(buffer, BUF_SIZE, session->game, 1);
   write_client(session->players[1].sock, buffer);
   puts("done handling this turn of game session");
}

void end_game(char *buffer, GameSession *session)
{
   snprintf(buffer, BUF_SIZE, "Game over! %s wins!\n", (session->game.stash[0] > session->game.stash[1]) ? session->players[0].name : session->players[1].name);

   SOCKET current = (session->game.stash[0] > session->game.stash[1]) ? session->players[0].sock : session->players[1].sock;
   SOCKET opponent = (session->game.stash[0] > session->game.stash[1]) ? session->players[1].sock : session->players[0].sock;

   write_client(current, buffer);
   write_client(opponent, buffer);

   closesocket(current);
   closesocket(opponent);

   session->active = false;
}

int check_if_player_is_connected(Client *clients, char *name, int nb_clients)
{

   for (int i = 0; i < nb_clients; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         return 1; // Player found
      }
   }
   return 0; // Player not found
}

GameSession *getSessionByClient(Client *client, GameSession *sessions, int session_count)
{
   for (int i = 0; i < session_count; i++)
   {
      if (sessions[i].players[0].sock == client->sock || sessions[i].players[1].sock == client->sock)
      {
         return &sessions[i];
      }
   }
   return NULL;
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE]; // initilisation du buffer de lecture et d'écriture

   char *userPwd[NB_USERS]; // nb max de user connectés
   int nbUsers = 0;         // nb de users dans la BD
   if (!loadUsers("users", userPwd, &nbUsers))
   {
      perror("loading users failed");
      exit(errno);
   }

   GameSession sessions[MAX_SESSIONS];
   int session_count = 0;

   Client waiting_clients[2];
   int waiting_count = 0;

   Client clients[MAX_CLIENTS];
   int nb_clients = 0; // nb de clients connectés

   int max_fd = sock;
   fd_set rdfs;

   while (1)
   {
      FD_ZERO(&rdfs);
      FD_SET(sock, &rdfs);

      for (int i = 0; i < waiting_count; i++)
      {
         FD_SET(waiting_clients[i].sock, &rdfs);
         if (waiting_clients[i].sock > max_fd)
         {
            max_fd = waiting_clients[i].sock;
         }
      }

      /* add socket of each client */
      for (int i = 0; i < nb_clients; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }
      for (int i = 0; i < session_count; i++)
      {
         if (sessions[i].active)
         {
            for (int j = 0; j < 2; j++)
            {
               FD_SET(sessions[i].players[j].sock, &rdfs);
               if (sessions[i].players[j].sock > max_fd)
               {
                  max_fd = sessions[i].players[j].sock;
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
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         if (read_client(csock, buffer) <= 0)
         {
            closesocket(csock);
            continue;
         }

         if (!authentification(buffer, userPwd, nbUsers))
         {
            write_client(csock, "Mauvais utilisateur ou mot de passe.\n");
            closesocket(csock);
            continue;
         }

         char *username = strtok(buffer, ";");

         if (check_if_player_is_connected(clients, username, nb_clients))
         {
            write_client(csock, "Vous êtes déjà connecté.\n");
            closesocket(csock);
            continue;
         }
         if (nb_clients == MAX_CLIENTS)
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
            strncpy(new_client.name, username, BUF_SIZE - 1);
            new_client.in_game = false;
            strcat(strcat(strcpy(buffer, "Bonjour "), new_client.name), ". Bienvenu sur Awale! \n");
            write_client(csock, buffer);

            clients[nb_clients] = new_client;
            nb_clients++;
            strcpy(buffer, new_client.name);
            puts(strcat(buffer, " connected"));
         }
      }
      else
      {
         for (int i = 0; i < nb_clients; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client *client = &clients[i];
               int c = read_client(client->sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  closesocket(client->sock);
                  remove_client(clients, i, &nb_clients);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, *client, nb_clients, buffer, 1);
               }
               if (client->in_game && strncmp(buffer, "/chat", 5) == 0) // check if client is in game
               {
                  strtok(buffer, " "); // ignore the command
                  char * msg = strtok(NULL, ""); // get the message
                  GameSession *session = getSessionByClient(client, sessions, session_count);
                  //ici on envoie à tous les clients de la session le message
                  puts (session->players[0].name);
                  puts(client->name);
                  if (strcmp(session->players[0].name,client->name)==0){
                      write_client(session->players[1].sock, msg);
                  }else{
                      write_client(session->players[0].sock, msg);
                  }
                  for (int j = 0; j < 6; j++) //voir si faut pas faire une variable nb_spec dans gameSession
                  {
                     if (session->spectators[j].sock != 0)
                     write_client(session->spectators[j].sock, msg); 
                  }
               }
               else if (client->in_game)
               {
                  GameSession *session = getSessionByClient(client, sessions, session_count);
                  if (session != NULL)
                  {
                     handle_game_session(buffer, c, session);
                  }
                  else
                  {
                     puts("session not found for client :");
                     puts(client->name);
                  }
                  // faire un get session by client pour pouvoir appeler handle game_session avec la session en question
               }
               // client launches game and is put on waiting list
               else if (strncmp(buffer, "/game", 5) == 0)
               {
                  puts("put client to true");
                  puts(client->name);
                  client->in_game = true;
                  waiting_clients[waiting_count++] = *client;
                  if (waiting_count == 2)
                  {
                     start_game_session(buffer, waiting_clients[0], waiting_clients[1], session_count, &sessions[session_count++]);
                     waiting_count = 0;
                  }
                  else
                  {
                     write_client(client->sock, "En attente d'un adversaire...\n");
                  }
               }
               else if (strncmp(buffer, "/msg", 4) == 0)
               {
                  send_private_message(clients, *client, nb_clients, buffer, 0);
               }
               else
               {
                  send_message_to_all_clients(clients, *client, nb_clients, buffer, 0);
               }
               break;
            }
         }
      }
   }

   end_connection(sock);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " (chat général) : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static void send_private_message(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   char message[BUF_SIZE];
   message[0] = 0;

   strtok(buffer, " ");                // ignore the command
   char *destUser = strtok(NULL, " "); // get the user who needs to receive the message
   char *msg = strtok(NULL, "");       // get the message
   for (int i = 0; i < actual; i++)
   {
      /* we only send to the destinator */

      if (strcmp(clients[i].name, destUser) == 0)
      {
         if (from_server == 0)
         {
            strcpy(message, "[message privé de ");
            strcat(message, sender.name);
            strncat(message, "] : ", sizeof message - strlen(message) - 1);
         }
         strcat(message, msg);
         write_client(clients[i].sock, message);
         return;
      }
   }
   // si on a pas return c'est que le destUser renseigné n'existe pas -----------------------------------------------------------------------> faut check aussi s'il est connecté!!!
   write_client(sender.sock, "This user is not connected or does not exist.\n");
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}

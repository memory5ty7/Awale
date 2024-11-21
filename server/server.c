#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../include/server.h"
#include "../include/game_session.h"

#define NB_USERS 10
#define NB_CHAR_PER_USERPWD 30
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

FILE *file;

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

void spectator_join_session(char *buffer, Client *newSpectator, GameSession *session)
{
   session->spectators[session->nb_spectators] = *newSpectator;
   session->nb_spectators++;

   newSpectator->in_game = true;

   puts("spectator in game");

   displayBoard(buffer, BUF_SIZE, session->game, 3);
   write_client(newSpectator->sock, buffer);
}

void start_game_session(char *buffer, Client player1, Client player2, int session_count, GameSession *session)
{
   session->nb_spectators = 0;
   if (session_count >= MAX_SESSIONS)
   {
      write_client(player1.sock, "Le serveur est plein.\n");
      write_client(player2.sock, "Le serveur est plein.\n");
      return;
   }

   session->players[0] = player1;
   session->players[1] = player2;
   session->game = initGame(1);
   session->active = true;

   time_t current_time = time(NULL);
   struct tm *local_time = localtime(&current_time);
   int hours = local_time->tm_hour;
   int minutes = local_time->tm_min;
   int seconds = local_time->tm_sec;
   char filename[16];
   char foldername[16];
   sprintf(foldername, "games/");
   snprintf(filename, sizeof(filename), "%02d:%02d:%02d\n", hours, minutes, seconds);
   strcat(foldername, filename);
   sprintf(filename, foldername);
   strcpy(session->fileName, filename);
   write_client(player1.sock, session->fileName);
   write_client(player2.sock, session->fileName);

   file = fopen(filename, "w+");
   if (file == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return false;
   }

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

   fprintf(file, "%s;%s;", player1.name, player2.name);
}

void handle_game_session(char *buffer, int len_buf, GameSession *session, Client *clients, int *nb_clients)
{
   Client *players;
   Client *current;
   Client *opponent;

   players = session->players;
   current = &players[session->game.current];
   opponent = &players[1 - session->game.current];

   if (len_buf <= 0)
   {
      snprintf(buffer, BUF_SIZE, "%s a quitté la partie. Fin de la partie.\n", current->name);

      for (int i = 0; i < session->nb_spectators; i++)
      {
         write_client(session->spectators[i].sock, buffer);
         session->spectators[i].in_game = false;
      }

      write_client(opponent->sock, buffer);
      session->players[0].in_game = false;
      session->players[1].in_game = false;

      closesocket(current->sock); // si on ferme le socket faut aussi remove le client de la liste des clients

      fprintf(file, "0");
      fclose(file);
      session->active = false;

      closesocket(current->sock);
      remove_client(current, getClientID(current->name, clients, &nb_clients), &nb_clients);
      return;
   }
   buffer[len_buf] = '\0';

   int choice = atoi(buffer);

   if (choice < 1 || choice > 6 || !session->game.movesAllowed[choice - 1])
   {
      write_client(current->sock, "Choix incorrect. Veuillez réessayer.\n");
      return;
   }

   if (!doMove(&(session->game), choice, session->game.current))
   {
      write_client(current->sock, "Choix incorrect. Veuillez réessayer.\n");
      return;
   }

   fprintf(file, "%d;", choice);

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

   for (int i = 0; i < session->nb_spectators; i++)
   {
      displayBoard(buffer, BUF_SIZE, session->game, 3);
      write_client(session->spectators[i].sock, buffer);
   }

   // puts("done handling this turn of game session");
}

void end_game(char *buffer, GameSession *session)
{
   Client winner = (session->game.stash[0] > session->game.stash[1]) ? session->players[0] : session->players[1];
   Client loser = (session->game.stash[0] > session->game.stash[1]) ? session->players[1] : session->players[0];

   snprintf(buffer, BUF_SIZE, "La partie est terminée! %s a gagné!\n", winner.name);

   write_client(winner.sock, buffer);
   write_client(loser.sock, buffer);

   fprintf(file, "0");
   fclose(file);
   updateScores("scores", winner.name, loser.name);

   session->players[0].in_game = false;
   session->players[1].in_game = false;

   for (int i = 0; i < session->nb_spectators; i++)
   {
      session->spectators[i].in_game = false;
   }

   session->active = false;
}

void updateScores(const char *filename, const char *winnerName, const char *loserName)
{
   FILE *fileScores = fopen(filename, "r");
   if (fileScores == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return false;
   }

   char lines[100][256];
   int lineCount = 0;
   char username[256];
   int victories, defeats;

   while (fgets(lines[lineCount], sizeof(lines[lineCount]), file))
   {
      if (sscanf(lines[lineCount], "%[^;];%d;%d", username, &victories, &defeats) == 3)
      {
         if (strcmp(username, winnerName) == 0)
         {
            victories++;
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s;%d;%d\n", username, victories, defeats);
         }
         else if (strcmp(username, loserName) == 0)
         {
            defeats++;
            snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s;%d;%d\n", username, victories, defeats);
         }
      }
      lineCount++;
   }

   fclose(fileScores);

   file = fopen(filename, "w");
   if (file == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return;
   }

   for (int i = 0; i < lineCount; i++)
   {
      fputs(lines[i], file);
   }

   fclose(file);
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

int getClientID(char *name, Client *clients, int nb_clients)
{
   for (int i = 0; i < nb_clients; i++)
   {
      if (strcmp(name, clients[i].name) == 0)
      {
         return i;
      }
   }
   return -1;
}
GameSession *getSessionByClient(Client *client, GameSession *sessions, int session_count)
{
   for (int i = 0; i < session_count; i++)
   {
      if (sessions[i].players[0].sock == client->sock || sessions[i].players[1].sock == client->sock)
      {
         return &sessions[i];
      }
      for (int j = 0; j < sessions[i].nb_spectators; j++)
      {
         if (sessions[i].spectators[j].sock == client->sock)
         {
            return &sessions[i];
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

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE]; // initilisation du buffer de lecture et d'écriture

   char userPwd[NB_USERS][NB_CHAR_PER_USERPWD]; // nb max de user connectés
   int nbUsers = 0;                             // nb de users dans la BD
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
         else if (nb_clients == MAX_CLIENTS)
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
            strcpy(buffer, "\n\nBonjour. Bienvenue sur Awale! \nVous pouvez vous connecter avec /login [username] [password]\nSi c'est la première fois que vous vous connectez, utilisez /register [username] [password]\n");
            write_client(csock, buffer);

            clients[nb_clients] = new_client;
            nb_clients++;
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
               /* logged_in player disconnected */
               if (c == 0 && client->logged_in && !client->in_game)
               {
                  closesocket(client->sock);
                  remove_client(clients, i, &nb_clients);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, *client, nb_clients, buffer, 1);
                  // server trace
                  puts(buffer);
               }
               else if (c == 0 && !client->logged_in && !client->in_game)
               { // client disconnected
                  closesocket(client->sock);
                  remove_client(clients, i, &nb_clients);
                  // server trace
                  puts("client disconnected");
               }
               else if (!client->logged_in) // le client doit se connecter ou s'inscrire avant de pouvoir faire des actions
               {
                  char *cmd = strtok(buffer, " ");       // get command
                  char *username = strtok(NULL, " ");    // get user name
                  char *pwd = strtok(NULL, " ");         // get password
                  char *checkifspace = strtok(NULL, ""); // check if there is a space after the password (space in pwd or too many arguments)
                  char userPass[NB_CHAR_PER_USERPWD];

                  if (username != NULL && pwd != NULL)
                     strcat(strcat(strcpy(userPass, username), ";"), pwd);
                  if (strcmp(cmd, "/login") == 0)
                  {
                     if (username == NULL || pwd == NULL)
                        write_client(client->sock, "Too few arguments.\nUsage : /login [username] [password]\n");
                     else if (checkifspace != NULL)
                        write_client(client->sock, "Too many arguments.\nUsage : /login [username] [password]\n");
                     else if (!authentification(userPass, userPwd, nbUsers))
                     {
                        write_client(client->sock, "Mauvais utilisateur ou mot de passe.\n");
                     }
                     else if (check_if_player_is_connected(clients, username, nb_clients))
                     {
                        write_client(client->sock, "Vous êtes déjà connecté.\n");
                        closesocket(client->sock);
                        remove_client(clients, i, &nb_clients);
                        // server trace
                        puts("client disconnected");
                     }
                     else // sinon le client peut bien se connecter
                     {
                        client->logged_in = true;
                        strcpy(client->name, username);
                        strcat(strcat(strcpy(buffer, "\n\nBonjour "), client->name), ". Vous êtes bien connecté sur Awale !\n");
                        write_client(client->sock, buffer);
                        // server trace
                        strcpy(buffer, client->name);
                        puts(strcat(buffer, " connected"));
                     }
                  }
                  else if (strcmp(cmd, "/register") == 0)
                  {
                     if (username == NULL || pwd == NULL)
                        write_client(client->sock, "Usage : /register [username] [password]\n");
                     else if (checkifspace != NULL)
                     {
                        write_client(client->sock, "Spaces are not allowed in password.\nUsage : /register [username] [password]\n");
                     }
                     else if (authentification(buffer, userPwd, nbUsers))
                     {
                        write_client(client->sock, "Cet utilisateur existe déjà.\nVeuillez-vous connecter avec /login\n");
                     }
                     else
                     {
                        file = fopen("users", "a");
                        if (file == NULL)
                        {
                           printf("Erreur à l'ouverture du fichier %s\n", "users");
                           return false;
                        }
                        fprintf(file, "%s;%s\n", username, pwd);
                        fclose(file);
                        strcat(strcat(strcpy(buffer, "\n\nBonjour "), client->name), ". Vous êtes bien inscrit sur Awale !\nVous pouvez maintenant vous connecter avec /login\n");
                        write_client(client->sock, buffer);
                        // server trace
                        strcpy(buffer, client->name);
                        puts(strcat(buffer, " registered"));
                     }
                  }
                  else
                  {
                     strcpy(buffer, "\nVeuillez rentrer une commande valide.\nVous pouvez vous connecter avec /login [username] [password]\nSi c'est la première fois que vous vous connectez, utilisez /register [username] [password]\n");
                     write_client(client->sock, buffer);
                  }
               }
               else // sinon le client est connecté et peut faire toutes les actions
               {
                  // input is a command
                  if (strncmp(buffer, "/", 1) == 0)
                  {
                     char *cmd = strtok(buffer, " ");

                     if (client->in_game && strcmp(cmd, "/chat") == 0)
                     {
                        char playerMessage[BUF_SIZE];

                        char messageToSend[BUF_SIZE];

                        char *token = strtok(NULL, "");

                        if (token != NULL)
                        {
                           strcpy(playerMessage, token);

                           snprintf(messageToSend, sizeof(messageToSend), "%s: %s", client->name, playerMessage);
                        }

                        GameSession *session = getSessionByClient(client, sessions, session_count);
                        if (session != NULL)
                        {
                           // ici on envoie à tous les clients de la session le message (sauf au client qui l'a envoyé)
                           for (int j = 0; j < 2; j++)
                           {
                              if (strcmp(session->players[j].name, client->name) != 0)
                              {
                                 write_client(session->players[j].sock, messageToSend);
                              }
                           }

                           for (int j = 0; j < session->nb_spectators; j++)
                           {
                              if (strcmp(session->spectators[j].name, client->name) != 0)
                              {
                                 write_client(session->spectators[j].sock, messageToSend);
                              }
                           }
                        }
                        else
                        {

                           // server trace

                           puts("Although in game, session was not found for client :");
                           puts(client->name);
                        }
                     }

                     else if (!client->in_game && strcmp(cmd, "/game") == 0)
                     {
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
                     else if (strcmp(cmd, "/showgames") == 0)
                     {
                        show_games(clients, *client);
                     }
                     else if (strcmp(cmd, "/join") == 0)
                     {
                        if (client->in_game)

                        {
                           write_client(client->sock, "Vous êtes déjà dans une partie.\n");
                           break;
                        }

                        char *destUser = strtok(NULL, "");
                        bool found = false;

                        if (destUser == NULL)
                           write_client(client->sock, "\nToo few arguments.\nUsage : /join [username]\n");
                        else if (strcmp(destUser, client->name) == 0)
                           write_client(client->sock, "Vous ne pouvez pas vous rejoindre vous-même...\nUsage : /join [username (other than yours -_-)]\n");
                        else
                        {
                           for (int i = 0; i < nb_clients; i++)
                           {
                              Client clientDest = clients[i];
                              if (strcmp(clientDest.name, destUser) == 0)
                              {
                                 found = true;
                                 GameSession *sessionAJoindre = NULL;

                                 sessionAJoindre = getSessionByClient(&clientDest, sessions, session_count);
                                 if (sessionAJoindre != NULL)
                                 {
                                    spectator_join_session(buffer, client, sessionAJoindre);
                                 }
                                 else
                                 {
                                    char message[BUF_SIZE];

                                    strcpy(message, "");
                                    strcat(message, destUser);
                                    strcat(message, " n'est pas dans une partie actuellement.\n");

                                    write_client(client->sock, message);
                                 }
                              }
                           }
                           if (!found)
                           {
                              write_client(client->sock, "L'utilisateur spécifié n'existe pas.\n");
                           }
                        }
                     }
                     else if (strcmp(cmd, "/msg") == 0)
                     {
                        send_private_message(clients, *client, nb_clients, buffer);
                     }
                     else if (strcmp(cmd, "/showusers") == 0)
                     {
                        show_users(clients, *client, nb_clients, sessions, session_count, buffer);
                     }
                     else if (strcmp(cmd, "/help") == 0)
                     {
                        write_client(client->sock, "\n\nListe des commandes disponibles :\n- /msg [user] [message] : envoyer un message privé\n- /quit : se déconnecter du serveur \n\nIn lobby :\n- /game : lancer une partie\n- /join [username] : rejoindre la gameroom d'un joueur pour assister à la partie\n- /showusers : affiche la liste des utilisateurs connectés ainsi que leur statut\n- /showgames : affiche la liste des parties terminées\n\nIn game :\n- /chat [message] : envoyer un message à tous les joueurs de la partie\n");
                     }
                     else if (strcmp(cmd, "/quit") == 0)
                     {
                        if (client->in_game && !isSpectator(client, getSessionByClient(client, sessions, session_count))) // on met le warning uniquement si c'est un joueur actif
                        {
                           if (isSpectator(client, getSessionByClient(client, sessions, session_count)))
                              puts("spectator is true");
                           write_client(client->sock, "Êtes-vous sûr de vouloir vous déconnecter ?\n[ATTENTION] Vous serez considéré perdant par forfait.\n (y/n)\n");
                           client->confirm_quit = true;
                        }
                        else if (client->logged_in)
                        {
                           write_client(client->sock, "Exiting server...\n");
                           closesocket(client->sock);
                           remove_client(clients, i, &nb_clients);
                           strncpy(buffer, client->name, BUF_SIZE - 1);
                           strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                           send_message_to_all_clients(clients, *client, nb_clients, buffer, 1);
                           // server trace
                           puts(buffer);
                        }
                        else
                        {
                           write_client(client->sock, "Exiting server...\n");
                           closesocket(client->sock);
                           remove_client(clients, i, &nb_clients);
                           // server trace
                           puts("client disconnected");
                        }
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
                           remove_client(clients, i, &nb_clients);
                           strncpy(buffer, client->name, BUF_SIZE - 1);
                           strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                           send_message_to_all_clients(clients, *client, nb_clients, buffer, 1);
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
                        GameSession *session = getSessionByClient(client, sessions, session_count);

                        if (session != NULL)
                        {
                           // Seul le joueur actif peut effectuer un move
                           if (strcmp(session->players[session->game.current].name, client->name) == 0)
                              handle_game_session(buffer, c, session, clients, nb_clients);
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
                     send_message_to_all_clients(clients, *client, nb_clients, buffer, 0);
                  }
                  break;
               }
            }
         }
      }
   }

   end_connection(sock);
}

static void end_connection(int sock) { closesocket(sock); }

void show_games(Client *clients, Client client)
{
   const char *folderPath = "games";     // Path to the folder
   struct dirent *entry;                 // Pointer for directory entry
   DIR *directory = opendir(folderPath); // Open the directory

   if (directory == NULL)
   {
      perror("Unable to open directory");
      return;
   }

   char message[BUF_SIZE];

   // Initialize the output buffer
   message[0] = '\0';

   strcat(message, "Liste des parties :\n");

   // Read and process all files in the folder
   while ((entry = readdir(directory)) != NULL)
   {
      char *filename = entry->d_name;

      // Build the full path to the file
      char filepath[512];
      snprintf(filepath, sizeof(filepath), "%s/%s", folderPath, filename);

      // Use stat to check if it is a regular file
      struct stat fileStat;
      if (stat(filepath, &fileStat) == -1 || !S_ISREG(fileStat.st_mode))
      {
         continue;
      }

      // Open the file
      FILE *file = fopen(filepath, "r");
      if (file == NULL)
      {
         perror("Error opening file");
         continue;
      }

      // Check if the file is not empty
      if (fileStat.st_size > 0)
      {
         // Read the last character of the file
         fseek(file, -1, SEEK_END);
         char lastChar = fgetc(file);

         if (lastChar == '0')
         {
            // Go back to the beginning of the file
            rewind(file);

            // Read the first line
            char line[BUF_SIZE];
            if (fgets(line, sizeof(line), file))
            {
               char filestr[BUF_SIZE];
               // Extract the first word before ';'
               char *player1 = strtok(line, ";");
               puts(player1);
               char *player2 = strtok(NULL, ";");
               puts(player2);
               sanitizeFilename(filename);
               sprintf(filestr, "- %s : %s VS. %s\n", filename, player1, player2);
               strcat(message, filestr);
            }
         }
      }

      fclose(file); // Close the file
   }

   closedir(directory); // Close the directory

   write_client(client.sock, message);
}

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

void show_users(Client *clients, Client sender, int nb_clients, GameSession *sessions, int session_count, const char *buffer)
{
   char message[BUF_SIZE];
   strcpy(message, "Liste des utilisateurs connectés:\n");
   for (int i = 0; i < nb_clients; i++)
   {
      if (clients[i].logged_in)
      {
         char curUser[BUF_SIZE];
         sprintf(curUser, "- %s : ", clients[i].name);
         char status[BUF_SIZE];
         if (clients[i].in_game)
         {
            GameSession *session = getSessionByClient(&clients[i], sessions, session_count);
            if (isSpectator(&clients[i], session))
            {
               char player1[BUF_SIZE];
               char player2[BUF_SIZE];
               strcpy(player1, session->players[0].name);
               strcpy(player2, session->players[1].name);
               sprintf(status, "Regarde la partie entre %s et %s\n", player1, player2);
            }
            else
            {
               char opponent[BUF_SIZE];
               if (strcmp(session->players[0].name, clients[i].name) == 0)
               {
                  strcpy(opponent, session->players[1].name);
               }
               else
               {
                  strcpy(opponent, session->players[0].name);
               }
               sprintf(status, "En partie contre %s\n", opponent);
            }
         }
         else
         {
            strcpy(status, "En ligne\n");
         }
         strcat(curUser, status);
         strcat(message, curUser);
      }
   }

   write_client(sender.sock, message);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender nor to not logged in clients*/
      if (clients[i].logged_in && sender.sock != clients[i].sock)
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

static void send_private_message(Client *clients, Client sender, int actual, const char *buffer)
{
   char message[BUF_SIZE];
   message[0] = 0;

   char *destUser = strtok(NULL, " "); // get the user who needs to receive the message
   char *msg = strtok(NULL, "");       // get the message
   puts(destUser);
   for (int i = 0; i < actual; i++)
   {
      /* we only send to the destinator and logged in players*/
      if (clients[i].logged_in && strcmp(clients[i].name, destUser) == 0)
      {
         strcpy(message, "[message privé de ");
         strcat(message, sender.name);
         strncat(message, "] : ", sizeof message - strlen(message) - 1);
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

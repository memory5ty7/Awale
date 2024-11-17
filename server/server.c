
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

<<<<<<< HEAD
#include "lobby.h"
#include "client_serv.h"
=======
#include "../include/lobby.h"
#include "../include/game_session.h"

#define NB_USERS 10
#define NB_CHAR_PER_USERPWD 30
>>>>>>> 49e30f6 (login working)

char *userPwd[NB_USERS];
int nbUsers; // nb de user dans la BD

<<<<<<< HEAD
int loadUsers(char* filename, int nbUsers, char** userPwd){
  FILE *fptr;
  fptr = fopen(filename,"r");

  if(fptr == NULL)
  {
    printf("Erreur à l'ouverture du fichier %s!",filename);   
    return nbUsers;           
  }

  for(int i=0;i<MAX_CLIENTS;i++){
    userPwd[i]=malloc(sizeof(char)*NB_CHAR_PER_USERPWD);
  }

  char line[NB_CHAR_PER_USERPWD];
  while(fgets(line,sizeof(line)/sizeof(char),fptr)){
   line[strcspn(line, "\r\n")] = '\0'; //retire le \r\n de la chaine
   strcpy(userPwd[nbUsers],line);
   nbUsers++;
  }
  fclose(fptr);
  return nbUsers;
}


bool authentification(char* userpassword, int nbUsers, char** userPwd){
   for (int i=0; i<nbUsers;i++){
      if (strcmp(userPwd[i],userpassword)==0){
         puts(userPwd[i]);
=======
char buffer[BUF_SIZE];

Client clients[MAX_CLIENTS];

bool add_client(Client client)
{
   for (int i = 0; i < sizeof(clients) / sizeof(Client); i++)
   {
      if (strcmp(clients[i].name, "") == 0)
      {
         strncpy(clients[i].name, client.name, BUF_SIZE - 1);
         clients[i].name[BUF_SIZE - 1] = '\0';
         clients[i].sock = client.sock;
>>>>>>> 49e30f6 (login working)
         return true;
      }
   }
   return false;
}

bool loadUsers(char *filename)
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
   nbUsers = 0;
   while (fgets(line, sizeof(line), fptr) && nbUsers < NB_USERS)
   {
      line[strcspn(line, "\n")] = '\0'; // Retire le \n
      strcpy(userPwd[nbUsers], line);
      nbUsers++;
   }

   fclose(fptr);
   return true;
}

bool authentification(char *userpassword)
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

static void app(void)
{
   SOCKET sock = init_connection();
   loadUsers("users");
   Client waiting_clients[2];
   int waiting_count = 0;

   fd_set rdfs;

   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      clients[i].sock = -1;
      strcpy(clients[i].name, "");
   }

   while (1)
   {
      FD_ZERO(&rdfs);
      FD_SET(sock, &rdfs);
      int max_fd = sock;

      for (int i = 0; i < waiting_count; i++)
      {
         FD_SET(waiting_clients[i].sock, &rdfs);
         if (waiting_clients[i].sock > max_fd)
         {
            max_fd = waiting_clients[i].sock;
         }
      }

      for (int i = 0; i < session_count; i++)
      {
         if (!sessions[i].active)
            continue;
         for (int j = 0; j < 2; j++)
         {
            FD_SET(sessions[i].players[j].sock, &rdfs);
            if (sessions[i].players[j].sock > max_fd)
            {
               max_fd = sessions[i].players[j].sock;
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

         if (!authentification(buffer))
         {
            write_client(csock, "Mauvais utilisateur ou mot de passe.\n");
            closesocket(csock);
            continue;
         }

         if (check_if_player_is_connected(csock))
         {
            write_client(csock, "Vous êtes déjà connecté.\n");
            closesocket(csock);
            continue;
         }

         Client new_client = {csock};

         if (!add_client(new_client))
         {
            write_client(csock, "Il n'y a plus de place sur le serveur.\n");
            closesocket(csock);
            continue;
         }

         char *username = strtok(buffer, ";");

         strncpy(new_client.name, username, BUF_SIZE - 1);

         waiting_clients[waiting_count++] = new_client;

         if (waiting_count == 2)
         {
            start_game_session(waiting_clients[0], waiting_clients[1]);
            waiting_count = 0;
         }
         else
         {
            write_client(csock, "En attente d'un adversaire...\n");
         }
      }

      for (int i = 0; i < session_count; i++)
      {
         if (!sessions[i].active)
            continue;

         GameSession *session = &sessions[i];
         Client *current = &session->players[session->currentPlayer];

         if (FD_ISSET(current->sock, &rdfs))
         {
            handle_game_session(session);
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

<<<<<<< HEAD
static void app(void)
{
   SOCKET sock = init_connection();
   
   char * userPwd [MAX_CLIENTS];
   int nbUsers = 0; //nb de user dans la BD
   nbUsers = loadUsers("./server/users", nbUsers, userPwd);
   if (nbUsers==0)
      {
      puts("Erreur lors du chargement des utilisateurs");
      exit(EXIT_FAILURE);
   }

   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its userPwd */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         puts("Authentification en cours...");
         if (!authentification(buffer, nbUsers, userPwd)){
            puts("Wrong user or password");
            write_client(csock, "Wrong user or password");
            continue;
         }

         char * name = strtok(buffer,";");
         char connectedUser [20] ="";
         puts(strcat(strcat(connectedUser,name)," connected"));
         write_client(csock, "Authentification succeed");

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, name, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  send_message_to_all_clients(clients, client, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

=======
>>>>>>> 49e30f6 (login working)
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
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
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

void start_game_session(Client player1, Client player2)
{

   if (session_count >= MAX_SESSIONS)
   {
      write_client(player1.sock, "Le serveur est plein.\n");
      write_client(player2.sock, "Le serveur est plein.\n");
      closesocket(player1.sock);
      closesocket(player2.sock);
      return;
   }

   GameSession *session = &sessions[session_count++];
   session->players[0] = player1;
   session->players[1] = player2;
   session->game = initGame(1);
   session->currentPlayer = 0;
   session->active = true;

   char msg[1024];

   snprintf(msg, sizeof(msg), "La partie contre %s commence !\n", player2.name);
   write_client(player1.sock, msg);

   snprintf(msg, sizeof(msg), "La partie contre %s commence !\n", player1.name);
   write_client(player2.sock, msg);

   displayBoard(buffer, BUF_SIZE, session->game, 0);
   write_client(session->players[0].sock, buffer);

   displayBoard(buffer, BUF_SIZE, session->game, 1);
   write_client(session->players[1].sock, buffer);

   snprintf(buffer, BUF_SIZE, "C'est à vous de jouer, %s. Choisissez une case.\n", session->players[session->currentPlayer].name);
   write_client(session->players[session->currentPlayer].sock, buffer);
}

void addChatMessage(ChatBuffer *chatBuffer, const char *message) {
    if (chatBuffer->messageCount < CHAT_BUFFER_SIZE) {
        strncpy(chatBuffer->messages[chatBuffer->messageCount], message, MAX_MESSAGE_LENGTH - 1);
        chatBuffer->messages[chatBuffer->messageCount][MAX_MESSAGE_LENGTH - 1] = '\0'; // Ensure null-termination
        chatBuffer->messageCount++;
    } else {
        // Overwrite the oldest message in a circular manner
        for (int i = 1; i < CHAT_BUFFER_SIZE; i++) {
            strncpy(chatBuffer->messages[i - 1], chatBuffer->messages[i], MAX_MESSAGE_LENGTH);
        }
        strncpy(chatBuffer->messages[CHAT_BUFFER_SIZE - 1], message, MAX_MESSAGE_LENGTH - 1);
        chatBuffer->messages[CHAT_BUFFER_SIZE - 1][MAX_MESSAGE_LENGTH - 1] = '\0';
    }
}

void handle_game_session(GameSession *session)
{
   Client *players = session->players;
   int currentPlayer = session->currentPlayer;
   Client *current = &players[currentPlayer];
   Client *opponent = &players[1 - currentPlayer];

   char buffer[BUF_SIZE];
   int result = read_client(current->sock, buffer);
   if (result <= 0)
   {
      snprintf(buffer, BUF_SIZE, "%s a quitté la partie. Fin de la partie.\n", current->name);
      write_client(opponent->sock, buffer);

      end_connection(current->sock);
      end_connection(opponent->sock);

      session->active = false;
      return;
   }
   //buffer[result] = '\0';

   if (strncmp(buffer, "/chat ", 6) == 0)
   {
      const char *chatMessage = buffer + 6;
      addChatMessage(&session->chatBuffer, chatMessage);

      snprintf(buffer, sizeof(buffer), "Chat: %s\n", chatMessage);
      for(int i=0;i<sizeof(players)/sizeof(Client);i++){
         if(i!=currentPlayer&&i!=0){
            write_client(players[i].sock, buffer);
         }
      }
      for(int i=0;i<sizeof(session->spectators)/sizeof(Client);i++){
         write_client(session->spectators[i].sock, buffer);
      }     
   } else {
      int choice = atoi(buffer);

      if (checkMove(&(session->game)))
      {
         end_game(session);
         return;
      }

      if (choice < 1 || choice > 6 || !session->game.movesAllowed[choice - 1])
      {
         write_client(current->sock, "Invalid move. Try again.\n");
         return;
      }

      if (!doMove(&(session->game), choice))
      {
         write_client(current->sock, "Invalid move 2. Try again.\n");
         return;
      }

      if (checkWin(session->game))
      {
         end_game(session);
         return;
      }

      displayBoard(buffer, BUF_SIZE, session->game, currentPlayer);
      write_client(session->players[session->currentPlayer].sock, buffer);

      session->currentPlayer = 1 - currentPlayer; // Changer de joueur
      snprintf(buffer, BUF_SIZE, "C'est à vous de jouer, %s. Choisissez une case.\n", players[session->currentPlayer].name);
      write_client(players[session->currentPlayer].sock, buffer);

      displayBoard(buffer, BUF_SIZE, session->game, opponent);
      write_client((*opponent).sock, buffer);
   }
}

void end_game(GameSession *session)
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

int check_if_player_is_connected(SOCKET sock)
{
   for (int i = 0; i < sizeof(clients) / sizeof(Client); i++)
   {
      if (clients[i].sock == sock)
      {
         return 1;
      }
   }
   return 0;
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}

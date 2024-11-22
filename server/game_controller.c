#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include "../include/server_state.h"

void start_game_session(ServerState *serverState, char *buffer, Client *player1, Client *player2, GameSession *session, int state)
{
   if (serverState->session_count >= MAX_SESSIONS)
   {
      write_client(player1->sock, "Le serveur est plein.\n");
      write_client(player2->sock, "Le serveur est plein.\n");
      return;
   }
   serverState->session_count++;
   session->nb_spectators = 0;
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
   write_client(player1->sock, session->fileName);
   write_client(player2->sock, session->fileName);

   session->file = fopen(filename, "w+");
   if (session->file == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return false;
   }

   char msg[1024];

   snprintf(msg, sizeof(msg), "La partie contre %s commence !\n", player2->name);
   write_client(player1->sock, msg);

   snprintf(msg, sizeof(msg), "La partie contre %s commence !\n", player1->name);
   write_client(player2->sock, msg);

   checkMove(&session->game, session->game.current);

   displayBoard(buffer, BUF_SIZE, session->game, 0);
   write_client(session->players[0]->sock, buffer);

   displayBoard(buffer, BUF_SIZE, session->game, 1);
   write_client(session->players[1]->sock, buffer);

   fprintf(session->file, "%s;%s;", player1->name, player2->name);
}

void handle_game_session(ServerState *serverState, char *buffer, int len_buf, GameSession *session)
{
   Client *current;
   Client *opponent;

   current = session->players[session->game.current];
   opponent = session->players[1 - session->game.current];

   if (len_buf <= 0)
   {
      snprintf(buffer, BUF_SIZE, "%s a quitté la partie. Fin de la partie.\n", current->name);

      for (int i = 0; i < session->nb_spectators; i++)
      {
         write_client(session->spectators[i]->sock, buffer);
         session->spectators[i]->in_game = false;
      }

      write_client(opponent->sock, buffer);
      session->players[0]->in_game = false;
      session->players[1]->in_game = false;

      fprintf(session->file, "0");
      fclose(session->file);
      session->active = false;

      closesocket(current->sock);
      remove_client(serverState, current);
      return;
   }
   buffer[len_buf] = '\0';

   int choice = atoi(buffer);

   if (choice < 1 || choice > 6 || !session->game.movesAllowed[choice - 1])
   {
      write_client(current->sock, "Choix incorrect. Veuillez réessayer.\n/chat [message] pour parler\n");
      return;
   }

   if (!doMove(&(session->game), choice, session->game.current))
   {
      write_client(current->sock, "Choix incorrect. Veuillez réessayer.\n/chat [message] pour parler\n");
      return;
   }

   fprintf(session->file, "%d;", choice);

   if (checkWin(session->game))
   {
      end_game(buffer, session);
      return;
   }

   session->game.current = 1 - session->game.current;

   if (checkMove(&session->game, session->game.current))
   {
      end_game(buffer, session);
      return;
   }

   displayBoard(buffer, BUF_SIZE, session->game, 0);
   write_client(session->players[0]->sock, buffer);

   displayBoard(buffer, BUF_SIZE, session->game, 1);
   write_client(session->players[1]->sock, buffer);

   for (int i = 0; i < session->nb_spectators; i++)
   {
      displayBoard(buffer, BUF_SIZE, session->game, 3);
      write_client(session->spectators[i]->sock, buffer);
   }

   // puts("done handling this turn of game session");
}

void end_game(char *buffer, GameSession *session)
{
   Client* winner = (session->game.stash[0] > session->game.stash[1]) ? session->players[0] : session->players[1];
   Client* loser = (session->game.stash[0] > session->game.stash[1]) ? session->players[1] : session->players[0];

   snprintf(buffer, BUF_SIZE, "La partie est terminée! %s a gagné!\n", winner->name);

   write_client(winner->sock, buffer);
   write_client(loser->sock, buffer);

   fprintf(session->file, "0");
   fclose(session->file);
   updateScores("scores", winner->name, loser->name);

   session->players[0]->in_game = false;
   session->players[1]->in_game = false;

   for (int i = 0; i < session->nb_spectators; i++)
   {
      session->spectators[i]->in_game = false;
   }

   session->active = false;
}

void spectator_join_session(char *buffer, Client *newSpectator, GameSession *session)
{
   session->spectators[session->nb_spectators] = newSpectator;
   session->nb_spectators++;

   newSpectator->in_game = true;

   puts("spectator in game");

   displayBoard(buffer, BUF_SIZE, session->game, 3);
   write_client(newSpectator->sock, buffer);
}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include "../include/server_state.h"

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

   while (fgets(lines[lineCount], sizeof(lines[lineCount]), fileScores))
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

   fileScores = fopen(filename, "w");
   if (fileScores == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return;
   }

   for (int i = 0; i < lineCount; i++)
   {
      fputs(lines[i], fileScores);
   }

   fclose(fileScores);
}

bool loadUsers(char *filename, ServerState* serverState)
{
   FILE *fptr = fopen(filename, "r");

   if (fptr == NULL)
   {
      printf("Erreur à l'ouverture du fichier %s\n", filename);
      return false;
   }

   serverState->nbUsers = 0;

   char line[NB_CHAR_PER_USERPWD];
   while (fgets(line, sizeof(line), fptr) && serverState->nbUsers < NB_USERS)
   {
      line[strcspn(line, "\r\n")] = '\0'; // Retire le \n
      strcpy(serverState->userPwd[serverState->nbUsers], line);
      
      (serverState->nbUsers)++;
   }

   fclose(fptr);
   return true;
}
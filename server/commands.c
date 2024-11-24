#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../include/server_state.h"
#include "../include/util.h"
#include "../include/game_controller.h"
#include "../include/replay_controller.h"

void cmd_chat(ServerState *serverState, Client *client, char *buffer)
{
    strtok(buffer, " "); // skip the command
    char playerMessage[BUF_SIZE];

    char messageToSend[BUF_SIZE];

    char *token = strtok(NULL, "");

    if (token != NULL)
    {
        strcpy(playerMessage, token);
        snprintf(messageToSend, sizeof(messageToSend), "%s: %s", client->name, playerMessage);
    }
    else
    {
        write_client(client->sock, "\nToo few arguments.\nUsage : /chat [message]\n");
        return;
    }

    GameSession *session = getSessionByClient(serverState, client);
    if (session != NULL)
    {
        // ici on envoie à tous les clients de la session le message (sauf au client qui l'a envoyé)
        for (int j = 0; j < 2; j++)
        {
            if (strcmp(session->players[j]->name, client->name) != 0)
            {
                write_client(session->players[j]->sock, messageToSend);
            }
        }

        for (int j = 0; j < session->nb_spectators; j++)
        {
            if (strcmp(session->spectators[j]->name, client->name) != 0)
            {
                write_client(session->spectators[j]->sock, messageToSend);
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

void cmd_game(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *mode = strtok(NULL, " ");
    char *opponent = strtok(NULL, "");
    int rotation = 0;
    if (mode == NULL)
    {
        write_client(client->sock, "Usage : /game [rotation(right/left)] [(optionnal) username]\n");
        return;
    }
    else if (strcmp(mode, "r") == 0 || strcmp(mode, "right") == 0 || strcmp(mode, "R") == 0 || strcmp(mode, "Right") == 0 || strcmp(mode, "RIGHT") == 0)
    {
        rotation = 1;
    }
    else if (strcmp(mode, "l") == 0 || strcmp(mode, "left") == 0 || strcmp(mode, "L") == 0 || strcmp(mode, "Left") == 0 || strcmp(mode, "LEFT") == 0)
    {
        rotation = -1;
    }
    else
    {
        write_client(client->sock, "Usage : /game [rotation(right/left)] [(optionnal) username]\n");
        return;
    }

    if (opponent == NULL)
    {
        client->in_queue = true;
        if (rotation == 1)
        {
            serverState->waiting_clients_r[serverState->waiting_count_r++] = client;
            if (serverState->waiting_count_r == 2)
            {
                serverState->waiting_clients_r[0]->in_game = true;
                serverState->waiting_clients_r[1]->in_game = true;
                serverState->waiting_clients_r[0]->in_queue = false;
                serverState->waiting_clients_r[1]->in_queue = false;

                start_game_session(serverState, buffer, serverState->waiting_clients_r[0], serverState->waiting_clients_r[1], &serverState->sessions[serverState->session_count], rotation);
                serverState->waiting_count_r = 0;
            }
            else
            {
                write_client(client->sock, "En attente d'un adversaire...\n");
            }
        }
        else
        {
            serverState->waiting_clients_l[serverState->waiting_count_l++] = client;
            if (serverState->waiting_count_l == 2)
            {
                serverState->waiting_clients_l[0]->in_game = true;
                serverState->waiting_clients_l[1]->in_game = true;
                serverState->waiting_clients_l[0]->in_queue = false;
                serverState->waiting_clients_l[1]->in_queue = false;

                start_game_session(serverState, buffer, serverState->waiting_clients_l[0], serverState->waiting_clients_l[1], &serverState->sessions[serverState->session_count], rotation);
                serverState->waiting_count_l = 0;
            }
            else
            {
                write_client(client->sock, "En attente d'un adversaire...\n");
            }
        }
    }
    else
    {
        for (int i = 0; i < serverState->nb_clients; i++)
        {
            Client *clientDest = &serverState->clients[i];
            if (strcmp(clientDest->name, opponent) == 0)
            {
                if (clientDest->in_game)
                {
                    strcpy(buffer, clientDest->name);
                    strcat(buffer, " est déjà dans une partie.\nAttendez qu'il ait fini pour lui renvoyer une invitation.\n");
                    write_client(client->sock, buffer);
                    return;
                }
                else if (strcmp(clientDest->name, client->name) == 0)
                {
                    write_client(client->sock, "Vous ne pouvez pas jouer contre vous-même...\nUsage : /game [username (other than yours -_-)]\n");
                    return;
                }
                else
                {
                    strcpy(buffer, client->name);
                    if (rotation == 1){
                        strcat(buffer, " vous a invité à jouer une partie 'counter-clockwise'. Acceptez-vous ?\n/accept pour accepter\n/decline pour refuser\n");
                    }
                    else{
                        strcat(buffer, " vous a invité à jouer une partie 'clockwise'. Acceptez-vous ?\n/accept pour accepter\n/decline pour refuser\n");
                    }
                    
                    write_client(clientDest->sock, buffer);

                    strcpy(buffer, "Une invitation a été envoyée à ");
                    strcat(buffer, clientDest->name);
                    strcat(buffer, ".\nEn attente de sa réponse...\n");
                    write_client(client->sock, buffer);

                    client->in_queue = true;
                    client->rotation = rotation;
                    strcpy(clientDest->challenger, client->name);
                    return;
                }
            }
        }
        write_client(client->sock, "L'utilisateur spécifié n'est pas connecté.\n");
        return;
    }
}

// accepte l'invitation d'un utilisateur
void cmd_accept(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(client->sock, "\nToo many arguments.\nUsage : /accept\n");
        return;
    }

    if (strcmp(client->challenger, "") != 0)
    {
        client->in_game = true;
        Client *challenger = &serverState->clients[getClientID(*serverState, client->challenger)];
        challenger->in_game = true;
        challenger->in_queue = false;
        start_game_session(serverState, buffer, client, challenger, &serverState->sessions[serverState->session_count], challenger->rotation);
        strcpy(client->challenger, "");
    }
    else
    {
        puts(client->challenger);
        write_client(client->sock, "Vous n'avez pas d'invitation en attente.\n");
    }
}

void cmd_decline(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(client->sock, "\nToo many arguments.\nUsage : /decline\n");
        return;
    }

    if (strcmp(client->challenger, "") != 0)
    {
        Client *challenger = &serverState->clients[getClientID(*serverState, client->challenger)];
        strcpy(buffer, client->name);
        strcat(buffer, " a refusé votre invitation.\n");
        puts("invitation refusée");
        write_client(challenger->sock, buffer);
        challenger->in_queue = false;
        puts(challenger->name);
        strcpy(client->challenger, "");
    }
    else
    {
        write_client(client->sock, "Vous n'avez pas d'invitation en attente.\n");
    }
}
// annuler une invitation ou une demande de partie
void cmd_cancel(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(client->sock, "\nToo many arguments.\nUsage : /cancel\n");
        return;
    }
    for (int i = 0; i < serverState->nb_clients; i++)
    {
        if (strcmp(serverState->clients[i].challenger, "") == 0)
            continue;
        if (strcmp(client->name, serverState->clients[i].challenger) == 0)
        {
            strcpy(buffer, client->name);
            strcat(buffer, " a annulé son invitation.\n");
            write_client(serverState->clients[i].sock, buffer);
            client->in_queue = false;
            strcpy(serverState->clients[i].challenger, "");
            return;
        }
    }
    // si on arrive ici, c'est que le client n'a pas envoyé d'invitation et donc qu'il est en random (/game sans arguments)
    client->in_queue = false;
    if (strcmp(serverState->waiting_clients_r[0]->name, client->name) == 0)
        serverState->waiting_count_r--;
    else
        serverState->waiting_count_l--;
    write_client(client->sock, "Recherche de partie annulée.\n");
}

void cmd_help(Client *client, char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(client->sock, "\nToo many arguments.\nUsage : /help\n");
        return;
    }
    write_client(client->sock, "\n\nListe des commandes disponibles :\n- /msg [user] [message] : envoyer un message privé\n- /quit : se déconnecter du serveur\n- /showusers : affiche la liste des utilisateurs connectés ainsi que leur statut\n\nIn lobby :\n- /game : lancer une partie\n- /join [username] : rejoindre la gameroom d'un joueur pour assister à la partie\n- /replay [game-id] : afficher une partie déjà terminée étape par étape\n- /showgames : affiche la liste des parties terminées\n\nIn queue (en attente d'un adversaire) :\n- /cancel : annule la recherche de partie ou l'invitation envoyée à un joueur\nIn game :\n- /chat [message] : envoyer un message à tous les joueurs de la partie\n");
}

void cmd_join(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *destUser = strtok(NULL, "");
    bool found = false;

    if (destUser == NULL)
        write_client(client->sock, "\nToo few arguments.\nUsage : /join [username]\n");
    else if (strcmp(destUser, client->name) == 0)
        write_client(client->sock, "Vous ne pouvez pas vous rejoindre vous-même...\nUsage : /join [username (other than yours -_-)]\n");
    else
    {
        for (int i = 0; i < serverState->nb_clients; i++)
        {
            Client clientDest = serverState->clients[i];
            if (strcmp(clientDest.name, destUser) == 0)
            {
                found = true;
                GameSession *sessionAJoindre = NULL;

                sessionAJoindre = getSessionByClient(serverState, &clientDest);
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
            return;
        }
    }
}

void cmd_login(ServerState *serverState, Client *client, char *buffer)
{
    strtok(buffer, " ");                // skip the command
    char *username = strtok(NULL, " "); // get user name
    char *pwd = strtok(NULL, " ");      // get password
    char userPass[NB_CHAR_PER_USERPWD];
    char *checkifspace = strtok(NULL, ""); // check if there is a space after the password (space in pwd or too many arguments)

    if (username != NULL && pwd != NULL)
        strcat(strcat(strcpy(userPass, username), ";"), pwd);

    if (username == NULL || pwd == NULL)
    {
        write_client(client->sock, "Too few arguments.\nUsage : /login [username] [password]\n");
    }
    else if (checkifspace != NULL)
    {
        write_client(client->sock, "Too many arguments.\nUsage : /login [username] [password]\n");
    }
    else if (!authentification(userPass, *serverState))
    {
        if (userExist(userPass, *serverState))
            write_client(client->sock, "Mauvais mot de passe.\n");
        else
            write_client(client->sock, "Cet utilisateur n'existe pas.\nVeuillez-vous inscrire avec /register\n");
    }
    else if (check_if_player_is_connected(*serverState, username))
    {
        write_client(client->sock, "Vous êtes déjà connecté.\n");
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

void cmd_msg(ServerState *serverState, Client *client, const char *buffer, int sender_index)
{
    char message[BUF_SIZE];
    message[0] = 0;
    strtok(buffer, " ");                // skip the command
    char *destUser = strtok(NULL, " "); // get the user who needs to receive the message
    char *msg = strtok(NULL, "");       // get the message

    if (destUser == NULL || msg == NULL)
    {
        write_client(client->sock, "\nToo few arguments.\nUsage : /msg [username] [message]\n");
        return;
    }

    if (strcmp(serverState->clients[sender_index].name, destUser) == 0)
    {
        write_client(serverState->clients[sender_index].sock, "Vous ne pouvez pas vous envoyer de message à vous-même...\nUsage : /msg [username (other than yours -_-)] [message]\n");
        return;
    }

    for (int i = 0; i < serverState->nb_clients; i++)
    {
        /* we only send to the destinator if he's logged in*/
        if (serverState->clients[i].logged_in && strcmp(serverState->clients[i].name, destUser) == 0)
        {
            strcpy(message, "[message privé de ");
            strcat(message, client->name);
            strncat(message, "] : ", sizeof message - strlen(message) - 1);
            strcat(message, msg);
            write_client(serverState->clients[i].sock, message);
            return;
        }
    }
    write_client(client->sock, "This user is not connected or does not exist.\n");
}

void cmd_quit(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(client->sock, "\nToo many arguments.\nUsage : /quit\n");
        return;
    }
    if (client->in_game) // on met le warning uniquement si c'est un joueur actif
    {
        if(!isSpectator(client, getSessionByClient(serverState, client)))
        {
            write_client(client->sock, "Êtes-vous sûr de vouloir vous déconnecter ?\n[ATTENTION] Vous serez considéré perdant par forfait.\n (y/n)\n");
            client->confirm_quit = true;
        }
    }
    else if (client->in_replay)
    {
        sprintf(buffer, "%s disconnected !\n", client->name);
        send_message_to_all_clients(*serverState, *client, buffer, 1);
        client->in_replay = false;

        closesocket(client->sock);
        remove_client(serverState, getClientID(*serverState, client->name));
    }
    else if (client->in_queue)
    {
        cmd_cancel(serverState, client, "/cancel");
        write_client(client->sock, "Exiting server...\n");
        closesocket(client->sock);
        strncpy(buffer, client->name, BUF_SIZE - 1);
        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
        send_message_to_all_clients(*serverState, *client, buffer, 1);
        remove_client(serverState, getClientID(*serverState, client->name));
        // server trace
        puts(buffer);
    }
    else if (client->logged_in)
    {
        GameSession *session = getSessionByClient(serverState, client);
        if (isSpectator(client, session)) // server trace
        {
            int to_remove = getSpectatorID(session, client->name);
            memmove(session->spectators + to_remove, session->spectators + to_remove + 1, (session->nb_spectators - to_remove - 1) * sizeof(Client *));
            session->nb_spectators--;
        }
        write_client(client->sock, "Exiting server...\n");
        closesocket(client->sock);
        strncpy(buffer, client->name, BUF_SIZE - 1);
        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
        send_message_to_all_clients(*serverState, *client, buffer, 1);
        remove_client(serverState, getClientID(*serverState, client->name));
        // server trace
        puts(buffer);
    }
    else
    {
        write_client(client->sock, "Exiting server...\n");
        closesocket(client->sock);
        remove_client(serverState, getClientID(*serverState, client->name));
        // server trace
        puts("client disconnected");
    }
}

void cmd_register(ServerState *serverState, Client *client, char *buffer)
{
    strtok(buffer, " ");                   // skip the command
    char *username = strtok(NULL, " ");    // get user name
    char *pwd = strtok(NULL, " ");         // get password
    char *checkifspace = strtok(NULL, ""); // check if there is a space after the password (space in pwd or too many arguments)

    if (username == NULL || pwd == NULL)
        write_client(client->sock, "Usage : /register [username] [password]\n");
    else if (checkifspace != NULL)
    {
        write_client(client->sock, "Spaces are not allowed in password.\nUsage : /register [username] [password]\n");
    }
    else if (userExist(buffer, *serverState))
    {
        write_client(client->sock, "Cet utilisateur existe déjà.\nVeuillez-vous connecter avec /login\n");
    }
    else
    {
        FILE *file = fopen("data/users", "a");
        if (file == NULL)
        {
            printf("Erreur à l'ouverture du fichier users\n");
            return false;
        }
        fprintf(file, "%s;%s\n", username, pwd);
        fclose(file);

        file = fopen("data/scores", "a");
        if (file == NULL)
        {
            printf("Erreur à l'ouverture du fichier scores\n");
            return false;
        }
        fprintf(file, "%s;%s;%s\n", username, "0", "0");
        fclose(file);

        sprintf(buffer, "\n\nBonjour %s. Vous êtes bien inscrit sur Awale !\nVous pouvez maintenant vous connecter avec /login\n", username);
        // strcat(strcat(strcpy(buffer, "\n\nBonjour "), username), ". Vous êtes bien inscrit sur Awale !\nVous pouvez maintenant vous connecter avec /login\n");
        write_client(client->sock, buffer);

        loadUsers("users", serverState); // on reload les users mtn qu'on en a rajouté un nouveau

        // server trace
        buffer[0] = 0;
        strcpy(buffer, username);
        puts(strcat(buffer, " registered"));
    }
}

void cmd_replay(ServerState *serverState, Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command

    char *gameId = strtok(NULL, "");
    if (gameId == NULL)
    {
        write_client(client->sock, "\nToo few arguments.\nUsage : /replay [game-id]\n");
        return;
    }

    char filename[BUF_SIZE];
    strcpy(filename, "games/");
    strcat(filename, gameId);

    size_t len = strlen(filename);
    if (len > 0 && filename[len - 1] == '\n')
    {
        filename[len - 1] = '\0';
    }

    puts(filename);

    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        write_client(client->sock, "La partie spécifiée est introuvable\n");
        return false;
    }

    char fileContent[1024];

    if (fgets(fileContent, sizeof(fileContent), file) == NULL)
    {
        write_client(client->sock, "Erreur de lecture du fichier.\n");
        fclose(file);
        return false;
    }

    fclose(file);

    char player1[NB_CHAR_PER_USERPWD];
    char player2[NB_CHAR_PER_USERPWD];

    char *token = strtok(fileContent, ";");
    if (token != NULL)
    {
        strcpy(player1, token);
    }
    else
    {
        write_client(client->sock, "Erreur de lecture du fichier.\n");
        return false;
    }

    token = strtok(NULL, ";");
    if (token != NULL)
    {
        strcpy(player2, token);
    }
    else
    {
        write_client(client->sock, "Erreur de lecture du fichier.\n");
        return false;
    }
    int moves[BUF_SIZE];
    int num_count = 0;

    while ((token = strtok(NULL, ";")) != NULL)
    {
        int temp = atoi(token);

        moves[num_count++] = temp;
    }

    client->in_replay = true;

    char message[NB_CHAR_PER_USERPWD];
    sprintf(message, "\n%s : %s VS. %s\nEnvoyez un message pour passer au tour suivant.\n", gameId, player1, player2);
    write_client(client->sock, message);

    start_replay_session(serverState, buffer, client, gameId, moves, num_count, &serverState->rSessions[serverState->rSession_count]);
}

void cmd_showgames(Client *client, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(client->sock, "\nToo many arguments.\nUsage : /showgames\n");
        return;
    }

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

    strcat(message, "\nListe des parties :\n");

    int nb_games = 0;

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
                nb_games++;
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

    if (nb_games == 0)
    {
        strcpy(message, "Il n'y a pas encore de parties qui ont été jouées.\nSoyez le premier à en faire une !\n");
    }

    write_client(client->sock, message);
}

void cmd_showusers(ServerState *serverState, Client *sender, const char *buffer)
{
    strtok(buffer, " "); // skip the command
    char *checkArg = strtok(NULL, "");
    if (checkArg != NULL)
    {
        write_client(sender->sock, "\nToo many arguments.\nUsage : /showusers\n");
        return;
    }

    // lis le fichier "scores" et pour les utilisateurs connectés, affiche leur score
    FILE *file = fopen("data/scores", "r");
    if (file == NULL)
    {
        perror("Unable to open scores file");
        return;
    }
    else
    {
        char message[BUF_SIZE];
        strcpy(message, "\nListe des utilisateurs connectés:\n");

        // on parcours chaque utilisateur et on affiche son nom s'il est connecté
        char line[BUF_SIZE];
        while (fgets(line, sizeof(line), file))
        {
            char *username = strtok(line, ";");
            char *victoire = strtok(NULL, ";");
            char *defaite = strtok(NULL, ";");
            defaite[strcspn(defaite, "\r\n")] = '\0'; // Retire le \n
            int i = getClientID(*serverState, username);
            if (i != -1) // le client est connecté
            {
                char curUser[BUF_SIZE];
                sprintf(curUser, "- %s [V:%s/D:%s]: ", serverState->clients[i].name, victoire, defaite);
                char status[BUF_SIZE];
                if (serverState->clients[i].in_game)
                {
                    GameSession *session = getSessionByClient(serverState, &serverState->clients[i]);
                    if (isSpectator(&serverState->clients[i], session))
                    {
                        char player1[BUF_SIZE];
                        char player2[BUF_SIZE];
                        strcpy(player1, session->players[0]->name);
                        strcpy(player2, session->players[1]->name);
                        sprintf(status, "Regarde la partie entre %s et %s\n", player1, player2);
                    }
                    else
                    {
                        char opponent[BUF_SIZE];
                        if (strcmp(session->players[0]->name, serverState->clients[i].name) == 0)
                        {
                            strcpy(opponent, session->players[1]->name);
                        }
                        else
                        {
                            strcpy(opponent, session->players[0]->name);
                        }
                        sprintf(status, "En partie contre %s\n", opponent);
                    }
                }
                else if (serverState->clients[i].in_replay)
                {
                    strcpy(status, "Regarde un replay\n");
                }
                else if (serverState->clients[i].in_queue)
                {
                    strcpy(status, "Cherche une partie\n");
                }
                else
                {
                    strcpy(status, "En ligne\n");
                }
                strcat(curUser, status);
                strcat(message, curUser);
            }
            else if (serverState->clients[i].in_replay)
            {
                strcpy(status, "Regarde un replay\n");
            }
            else if (serverState->clients[i].in_queue)
            {
                strcpy(status, "Cherche une partie\n");
            }
            else
            {
                strcpy(status, "En ligne\n");
            }
            strcat(curUser, status);
            strcat(message, curUser);
        }
        fclose(file);
        write_client(sender->sock, message);
    }
}
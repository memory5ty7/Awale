CC = gcc

CFLAGS = -Wall

SRC_GAME = ./game/game.c
SRC_SERVER = ./server/server.c $(SRC_GAME)
SRC_PLAYER = ./client/client.c $(SRC_GAME)

OBJ_GAME = $(SRC_GAME:.c=.o)
OBJ_SERVER = $(SRC_SERVER:.c=.o)
OBJ_PLAYER = $(SRC_PLAYER:.c=.o)

EXE_SERVER = server/server
EXE_LOBBY = lobby/lobby
EXE_PLAYER = client/joueur

all: $(EXE_SERVER) $(EXE_PLAYER)

$(EXE_SERVER): $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_SERVER)

$(EXE_LOBBY): $(OBJ_LOBBY)
	$(CC) $(CFLAGS) -o $@ $(OBJ_LOBBY)

$(EXE_PLAYER): $(OBJ_PLAYER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_PLAYER)

%.o: %.c game.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f \*.o $(EXE_SERVER) $(EXE_PLAYER) $(EXE_LOBBY)

rebuild:
	clean all

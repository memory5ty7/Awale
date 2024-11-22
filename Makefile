CC = gcc

CFLAGS = -w

SRC_GAME = ./game/game.c
SRC_COMMANDS = ./server/commands.c
SRC_GAME_CONTROLLER = ./server/game_controller.c
SRC_IO = ./server/io.c
SRC_UTIL = ./server/util.c

SRC_DEPENDENCIES = $(SRC_COMMANDS) $(SRC_GAME_CONTROLLER) $(SRC_IO) $(SRC_UTIL)

SRC_SERVER = ./server/server.c $(SRC_GAME) $(SRC_DEPENDENCIES)
SRC_PLAYER = ./client/player.c $(SRC_GAME)

OBJ_GAME = $(SRC_GAME:.c=.o)
OBJ_SERVER = $(SRC_SERVER:.c=.o)
OBJ_PLAYER = $(SRC_PLAYER:.c=.o)

EXE_SERVER = server/server
EXE_PLAYER = client/player

all: $(EXE_SERVER) $(EXE_PLAYER)

$(EXE_SERVER): $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_SERVER)

$(EXE_PLAYER): $(OBJ_PLAYER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_PLAYER)

%.o: %.c game.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find . -type f -name "*.o" -exec rm -f {} +
	rm -f $(EXE_SERVER) $(EXE_PLAYER)
	
rebuild:
	clean all

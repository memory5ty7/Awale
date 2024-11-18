CC = gcc

CFLAGS = -w

SRC_GAME = ./game/game.c
SRC_PLAYER = ./client/player.c $(SRC_GAME)
SRC_SERVER = ./server/server.c $(SRC_GAME)

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

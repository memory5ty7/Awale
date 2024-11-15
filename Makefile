CC = gcc

CFLAGS = -Wall

SRC_GAME = game.c
SRC_SERVER = socket_server.c $(SRC_GAME)
SRC_PLAYER = socket.client.c $(SRC_GAME)

OBJ_GAME = $(SRC_GAME:.c=.o)
OBJ_SERVER = $(SRC_SERVER:.c=.o)
OBJ_PLAYER = $(SRC_PLAEYR:.c=.o)

EXE_SERVER = serveur
EXE_PLAYER = joueur

all: $(EXE_SERVER) $(EXE_PLAYER)

$(EXE_SERVER): $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_SERVER)

$(EXE_PLAYER): $(OBJ_PLAYER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_PLAYER)

%.o: %.c game.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXE_SERVER) $(EXE_PLAYER)

rebuild:
	clean all

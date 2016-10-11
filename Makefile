CC = gcc
CCFLAGS = -g -Wall
LDFLAGS = -lSDL2

BIN = gol
SRC = main.c
INC = 

$(BIN): $(SRC) $(INC)
	$(CC) $(SRC) -o $(BIN) $(CCFLAGS) $(LDFLAGS) 


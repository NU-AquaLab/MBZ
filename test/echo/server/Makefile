CC=gcc
CFLAGS=-Wall
BIN=server

all: $(BIN)

$(BIN): server.c
	$(CC) $(CFLAGS) server.c -o $(BIN)

.PHONY: clean

clean: 
	rm -f $(BIN)

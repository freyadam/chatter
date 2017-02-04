
C_FLAGS=-Wall
CC=gcc
SRC=main.c
OBJ=$(SRC:.c=.o)

.PHONY : all
all: client server

client: client.o
	$(CC) $(C_FLAGS) -o $@ $^

client.o : client.c
	$(CC) $(C_FLAGS) -c -o $@ $^

server: server.o
	$(CC) $(C_FLAGS) -o $@ $^

server.o : server.c
	$(CC) $(C_FLAGS) -c -o $@ $^


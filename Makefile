
C_FLAGS=-Wall
LD_FLAGS=-pthread
CC=gcc
OBJ=$(SRC:.c=.o)

.PHONY : all
all: bin/client bin/server

bin/client: client.o 
	$(CC) $(C_FLAGS) $(LD_FLAGS) -o $@ $^

client.o : client.c
	$(CC) $(C_FLAGS) -c -o $@ $^

bin/server: server.o
	$(CC) $(C_FLAGS) $(LD_FLAGS) -o $@ $^

server.o : server.c
	$(CC) $(C_FLAGS) -c -o $@ $^


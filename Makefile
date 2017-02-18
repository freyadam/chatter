

CLIENT_SRC=client.c client_comm.c proto.c thread_common.c
SERVER_SRC=server.c accept.c comm.c menu.c proto.c signal.c thread_common.c
C_FLAGS=-Wall
LD_FLAGS=-pthread
CC=gcc
OBJ=$(SRC:.c=.o)

.PHONY : all
all: bin/client bin/server

bin/client: $(CLIENT_SRC:.c=.o)
	$(CC) $(C_FLAGS) $(LD_FLAGS) -o $@ $^

bin/server: $(SERVER_SRC:.c=.o)
	$(CC) $(C_FLAGS) $(LD_FLAGS) -o $@ $^

%.o : %.c
	$(CC) $(C_FLAGS) -c -o $@ $^

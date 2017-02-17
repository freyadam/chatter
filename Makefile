

CLIENT_SRC=client.c client_comm.c proto.c thread_common.c
SERVER_SRC=server.c accept.c comm.c menu.c proto.c signal.c thread_common.c
CFLAGS=-Wall
LDFLAGS=-pthread
CC=gcc
OBJ=$(SRC:.c=.o)

.PHONY : all
all: bin/client bin/server

bin/client: $(CLIENT_SRC:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

bin/server: $(SERVER_SRC:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

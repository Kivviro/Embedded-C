CC = gcc
CFLAGS = -std=c17 -D_POSIX_C_SOURCE=200809L
TARGETS = server client

all: $(TARGETS)

server: producer_server.c
	$(CC) $(CFLAGS) producer_server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
CC = gcc
CFLAGS = -std=c17 -D_POSIX_C_SOURCE=200809L

TARGETS = server client

all: $(TARGETS)

server: sp_tcp_server.c
	$(CC) $(CFLAGS) sp_tcp_server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f $(TARGETS)

re: clean all

.PHONY: all clean
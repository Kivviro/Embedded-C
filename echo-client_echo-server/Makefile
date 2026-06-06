CC = gcc
CFLAGS = -std=c17
TARGETS = server/server.out client/client.out
BIN = server/server.out client/client.out

all: $(TARGETS)

server/server.out: server/server.c
	$(CC) $(CFLAGS) $< -o $@

client/client.out: client/client.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean
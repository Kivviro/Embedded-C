CC = gcc
CFLAGS = -std=c17
TARGETS = server client
BIN = server.out client.out

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server.out

client: client.c
	$(CC) $(CFLAGS) client.c -o client.out

clean:
	rm -f $(BIN) *.o

.PHONY: all clean

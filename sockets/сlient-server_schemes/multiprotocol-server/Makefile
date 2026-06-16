CC = gcc
CFLAGS = -std=c17
TARGETS = server tcp_client udp_client

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server

tcp_client: tcp_client.c
	$(CC) $(CFLAGS) tcp_client.c -o tcp_client

udp_client: udp_client.c
	$(CC) $(CFLAGS) udp_client.c -o udp_client

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
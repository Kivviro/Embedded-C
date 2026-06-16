CC = gcc
CFLAGS = -std=c17
TARGETS = server client_udp client_udp_ip client_udp_ip_eth
BIN = server.out udp/client.out udp_ip/client.out udp_ip_eth/client.out

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server.out

client_udp: udp/client.c
	$(CC) $(CFLAGS) udp/client.c -o udp/client.out

client_udp_ip: udp_ip/client.c
	$(CC) $(CFLAGS) udp_ip/client.c -o udp_ip/client.out

client_udp_ip_eth: udp_ip_eth/client.c
	$(CC) $(CFLAGS) udp_ip_eth/client.c -o udp_ip_eth/client.out

clean:
	rm -f $(BIN) *.o

.PHONY: all clean

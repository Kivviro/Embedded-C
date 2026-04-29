CC = gcc
CFLAGS = -std=c17
TARGETS = sniffer

all: $(TARGETS)

sniffer: simple_udp_sniffer.c
	$(CC) $(CFLAGS) simple_udp_sniffer.c -o sniffer

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
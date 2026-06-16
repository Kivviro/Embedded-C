CC = gcc
CFLAGS = -std=c17 -pthread -O2
TARGETS = dmanager.out
BIN = dmanager.out

all: $(TARGETS)

dmanager.out: driver_manager.c
	$(CC) $(CFLAGS) $< -o $@
	
clean:
	rm -f $(TARGETS)

.PHONY: all clean
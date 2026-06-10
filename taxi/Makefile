CC = gcc
CFLAGS = -std=c17
TARGETS = taxi.out
BIN = taxi.out

all: $(TARGETS)

taxi.out: taxi.c
	$(CC) $(CFLAGS) $< -o $@
	
clean:
	rm -f $(TARGETS)

.PHONY: all clean
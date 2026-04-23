# makefile для базовых сокето
CC = gcc
CFLAGS = -std=c17

SRCS := $(wildcard tcp/*/*.c udp/*/*.c)
BINS := $(SRCS:.c=)

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BINS)

.PHONY: all clean
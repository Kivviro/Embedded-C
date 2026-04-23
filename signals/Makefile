CC = gcc
CFLAGS = -std=c17 -D_POSIX_C_SOURCE=200809L

TARGETS = block_sigint event_loop override_usr1 sender

all: $(TARGETS)

block_sigint: block_sigint.c
	$(CC) $(CFLAGS) -o $@ $<

event_loop: event_loop.c
	$(CC) $(CFLAGS) -o $@ $<

override_usr1: override_usr1.c
	$(CC) $(CFLAGS) -o $@ $<

sender: sender.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm -f $(TARGETS)

.PHONY: all clean
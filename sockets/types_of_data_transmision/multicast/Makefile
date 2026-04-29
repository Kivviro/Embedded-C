CC = gcc
CFLAGS = -std=c17
TARGETS = reciever sender

all: $(TARGETS)

reciever: reciever.c
	$(CC) $(CFLAGS) reciever.c -o reciever

sender: sender.c
	$(CC) $(CFLAGS) sender.c -o sender

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean

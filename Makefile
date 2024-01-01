CC = gcc
CFLAGS = -Wall -pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	$(CC) $^ -o $@

clean:
	rm -f server client

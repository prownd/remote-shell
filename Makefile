CC = gcc
CFLAGS = -ggdb3  -fPIC -std=gnu99 -D_GNU_SOURCE -DPROV 

all: server client

server: server.c
	$(CC) -o server server.c $(CFLAGS)

client: client.c
	$(CC) -o client client.c $(CFLAGS)

install:
	cp -f  client server /usr/bin

clean:
	rm -f client server

uninstall:
	rm -f  /usr/bin/client
	rm -f  /usr/bin/server

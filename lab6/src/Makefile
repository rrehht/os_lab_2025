CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: client server

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c -o utils.o

libutils.a: utils.o
	ar rcs libutils.a utils.o

client: client.c utils.h libutils.a
	$(CC) $(CFLAGS) client.c -L. -lutils -o client

server: server.c utils.h libutils.a
	$(CC) $(CFLAGS) server.c -L. -lutils -o server

clean:
	rm -f client server utils.o libutils.a

.PHONY: all clean
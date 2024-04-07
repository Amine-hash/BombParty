CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm

all: client
serveur: client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

serveur.o: client.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f client client.o
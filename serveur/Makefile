CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm

all: serveur
serveur: serveur.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

serveur.o: serveur.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f serveur serveur.o
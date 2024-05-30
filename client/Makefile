CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm
COMMON_DIR = ../common
DICO_SRC = $(COMMON_DIR)/gestion_dictionnaire.c

all: client
client: client.o $(COMMON_DIR)/gestion_dictionnaire.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client.o: client.c
	$(CC) $(CFLAGS) -c $<

gestion_dictionnaire.o: $(DICO_SRC)
	$(CC) $(CFLAGS) -c $< -o $(COMMON_DIR)/gestion_dictionnaire.o

clean:
	rm -f client client.o $(COMMON_DIR)/gestion_dictionnaire.o
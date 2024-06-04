CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm
COMMON_DIR = ../common
DICO_SRC = $(COMMON_DIR)/gestion_dictionnaire.c
TUBE_SRC = $(COMMON_DIR)/tubes.c
all: client
client: client.o $(COMMON_DIR)/gestion_dictionnaire.o $(COMMON_DIR)/tubes.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client.o: client.c
	$(CC) $(CFLAGS) -c $<

gestion_dictionnaire.o: $(DICO_SRC)
	$(CC) $(CFLAGS) -c $< -o $(COMMON_DIR)/gestion_dictionnaire.o 

tubes.o: $(TUBE_SRC)
	$(CC) $(CFLAGS) -c $< -o $(COMMON_DIR)/tubes.o

clean:
	rm -f client client.o $(COMMON_DIR)/gestion_dictionnaire.o $(COMMON_DIR)/tubes.o
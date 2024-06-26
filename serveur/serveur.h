#ifndef SERVER_H
#define SERVER_H

#include "../common/common.h"
#define FILENAME "../dictionnaire/Français_v2.txt"
#define MIN_TEMPS_BOMBE 15 // Le temps minimum avant que la bombe explose
#define MAX_TEMPS_BOMBE 30 // Le temps maximum avant que la bombe explose

HashTable* dictionnaire;
HashTable* groupes_lettres;
char* dictionnaire_array[TABLE_SIZE];
int taille_dictionnaire_array;
int taille_dictionnaire;
Partie parties[MAX_PARTIES];
int nombre_parties = 0;


void send_confirmation(const char *tube_name, const char *message);
void handle_new_game(char *buffer); 
void handle_join_game(char *buffer, const char *tube_name, pid_t pid);
void notify_players(Partie *partie, const char *pseudo);
void *client_thread(void *arg);
void signal_handler(int sig, siginfo_t *info, void *context) ;

void *game_thread(void *arg);

#endif /* SERVER_H */

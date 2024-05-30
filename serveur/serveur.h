#ifndef SERVER_H
#define SERVER_H

#include "../common/common.h"
#define FILENAME "../dictionnaire/Français_v2.txt"

HashTable* dictionnaire;
HashTable* groupes_lettres;
char* dictionnaire_array[TABLE_SIZE];
int taille_dictionnaire_array;
int taille_dictionnaire;
Partie parties[MAX_PARTIES];
int nombre_parties = 0;


void create_named_pipe(const char *tube_name);
void send_confirmation(const char *tube_name, const char *message);
void handle_new_game(char *buffer, const char *tube_name);
void handle_join_game(char *buffer, const char *tube_name, pid_t pid);
void notify_players(Partie *partie, const char *pseudo);
void *client_thread(void *arg);
void signal_handler(int sig, siginfo_t *info, void *context) ;

void *game_thread(void *arg);

#endif /* SERVER_H */

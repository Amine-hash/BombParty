#ifndef CLIENT_H
#define CLIENT_H
#include "../common/common.h"
#define MIN_TEMPS_BOMBE 15 // Le temps minimum avant que la bombe explose
#define MAX_TEMPS_BOMBE 30 // Le temps maximum avant que la bombe explose

void *client_game_handler(void *arg);
Partie *recuperer_infos_partie(int partie_id);
void signal_handler(int signum) ;
#endif 
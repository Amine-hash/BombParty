#ifndef GAME_STRUCTURES_H
#define GAME_STRUCTURES_H
#define MAX_GAMES 100
#define MAX_LETTERS 10
#define MAX_PLAYERS 100
#define MAX_NAME_LENGTH 20
#define MAX_PARTIES 100
#define MAX_PLAYERS 100
#define DEFAULT_VIES 3
#define DEFAULT_JOUEURS 4
#define EN_ATTENTE 1
#define EN_JEU 2
#define PERDU 3
#define PARTIE_EN_ATTENTE_JOUEURS 4
#define PARTIE_DEMARREE 5
#define PARTIE_TERMINEE 6
#define EXPLOSION 7
// Structure pour représenter un joueur
struct Joueur {
    char pseudo[MAX_NAME_LENGTH];
    int id;
    int nombre_vie;
    int etat;
    int flag_tour;
    char tube_name[20];
};
#define Joueur struct Joueur
// Structure pour représenter une partie
struct Partie {
    int nombre_joueur_courant;
    int nombre_joueur_max;
    Joueur joueurs[MAX_PLAYERS];
    int id;
    int etat;
    int nombre_vies;
};
#define Partie struct Partie
typedef struct Message {
    long type;
    Partie partie;
} Message;

struct thread_bomb_args {
    pthread_t tid_partie;
    Partie* partie;
    float *temps_bombe;
};
#define thread_bomb_args struct thread_bomb_args
#define SHARED_MEMORY_SIZE (sizeof(pid_t) + sizeof(int) + MAX_GAMES * sizeof(Partie))

#endif /* GAME_STRUCTURES_H */

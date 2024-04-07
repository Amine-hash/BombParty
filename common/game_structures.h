#ifndef GAME_STRUCTURES_H
#define GAME_STRUCTURES_H

#define MAX_LETTERS 10
#define MAX_PLAYERS 100
#define MAX_NAME_LENGTH 20

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    int lives;
} Player;

typedef struct {
    int currentPlayerId;
    int remainingTime;
    char letters[MAX_LETTERS];
    Player players[MAX_PLAYERS];
    int numPlayers;
} GameState;

#endif /* GAME_STRUCTURES_H */

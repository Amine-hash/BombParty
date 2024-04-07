#ifndef SERVER_H
#define SERVER_H

#include "../common/common.h"


// Fonction pour créer une nouvelle partie
int createGame(int gameId, int maxPlayers, int difficulty, ...);

// Fonction pour démarrer une partie
void startGame(int gameId);

// Fonction pour terminer une partie
void endGame(int gameId);

// Fonction pour distribuer les lettres aux joueurs
void distributeLetters(int gameId);

// Fonction pour passer la bombe au joueur suivant
void passBomb(int gameId, int currentPlayerId);

// Fonction pour initialiser les vies des joueurs
void initializeLives(int gameId, int playerIds[], int numPlayers);

// Fonction pour mettre à jour le nombre de vies restantes d'un joueur
void updateRemainingLives(int gameId, int playerId, int remainingLives);

// Fonction pour détecter la perte de vie d'un joueur
void detectLifeLoss(int gameId, int playerId);

// Fonction pour vérifier si un joueur est le dernier avec des vies
void endGameIfLastPlayer(int gameId);

#endif /* SERVER_H */

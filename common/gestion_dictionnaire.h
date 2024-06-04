#ifndef GESTION_DICTIONNAIRE_H
#define GESTION_DICTIONNAIRE_H
#include "common.h"

#define TABLE_SIZE 1000000
#define MAX_LONGUEUR_MOT 100
// Structure pour représenter un nœud de la liste chaînée
struct node {
    char key[MAX_LONGUEUR_MOT];
    int value;
    struct node *next;
};
typedef struct node node;
// Structure pour représenter une table de hachage
struct HashTable {
    node *head[TABLE_SIZE];
    int count[TABLE_SIZE];
};
void convertir_caracteres_speciaux(char *mot) ;
typedef struct HashTable HashTable;
void freeTable(HashTable* table) ;

HashTable* createTable();
void insert(HashTable* table, char* key, int value);
node* search(HashTable* table, char* key); 
int hashFunction(char* key);
int charger_dictionnaire_array(const char *nom_fichier, char *dictionnaire[]);
void freeTable(HashTable* table);
int charger_dictionnaire(const char *nom_fichier, HashTable* dictionnaire);
int verifier_presence_groupe_lettres(const char *groupe_lettres, HashTable* dictionnaire);
void convertir_caracteres_speciaux(char *mot);
node* lookup(HashTable* table, char* key);
int generer_groupe_lettres(HashTable* groupe_lettres, HashTable* dictionnaire);
void generer_groupe_lettre_aleatoire(HashTable *groupe_lettres, char* groupe_lettre_aleatoire);
void generer_groupe_lettre_aleatoire_v2(HashTable *dictionnaire, char* groupe_lettre_aleatoire);
int verifier_presence_groupe_lettres_array(const char *groupe_lettres, char *dictionnaire[], int taille_dictionnaire);
void generer_groupe_lettres_array(char *groupe_lettres, char *dictionnaire[], int taille_dictionnaire, HashTable* groupe_lettres_utilisé);
int mot_existe(char *mot, HashTable* dictionnaire); 

#endif /* GESTION_DICTIONNAIRE_H */

#include "gestion_dictionnaire.h"

node* createNode(char* key, int value) {
    node* newNode;
    newNode = (node*)malloc(sizeof(node));
    strcpy(newNode->key, key);
    newNode->value = value;
    newNode->next = NULL;
    return newNode;
}

// Créer une table de hachage
HashTable* createTable() {
    HashTable* newTable;
    newTable = (HashTable*)malloc(sizeof(HashTable));
    int i;
    for(i = 0; i < TABLE_SIZE; i++) {
        newTable->head[i] = NULL;
        newTable->count[i] = 0;
    }
    return newTable;
}
// Fonction de hachage simple
int hashFunction(char* key) {
    unsigned int sum = 0;
    for (int i = 0; key[i]; i++)
        sum += (unsigned char)key[i];
    return sum % TABLE_SIZE;
}
// Fonction pour charger le dictionnaire dans une liste de mots
int charger_dictionnaire_array(const char *nom_fichier, char *dictionnaire[]) {
    printf("Chargement du dictionnaire...%s\n", nom_fichier);
    FILE *file = fopen(nom_fichier, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1;
    }

    int index = 0;
    // mot en UTF-8
    char mot[MAX_LONGUEUR_MOT];

    while (fscanf(file, "%s", mot) == 1) {
        // Convertir les caractères spéciaux UTF-8 en caractères classiques
        //convertir_caracteres_speciaux(mot);
        convertir_caracteres_speciaux(mot);
        dictionnaire[index] = strdup(mot);

        index++;
    }

    fclose(file);
    FILE *file2 = fopen("../dictionnaire/dictionnaire_formatted.txt", "w");
    if (file2 == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1;
    }
    for (int i = 0; i < index; i++) {
        fprintf(file2, "%s\n", dictionnaire[i]);
    }
    fclose(file2);
    
    return index;
}

// Insérer un élément dans la table
void insert(HashTable* table, char* key, int value) {
    int hashIndex = hashFunction(key);
    node* newNode =  createNode(key, value);
    // Ajouter le nœud à la tête de la liste chaînée
    newNode->next = table->head[hashIndex];
    table->head[hashIndex] = newNode;
    table->count[hashIndex]++;
}

// Rechercher un élément dans la table
node* search(HashTable* table, char* key) {
    int hashIndex = hashFunction(key);
    node* temp = table->head[hashIndex];
    // Parcourir la liste chaînée
    while(temp) {
        if(strcmp(temp->key, key) == 0){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}
void freeTable(HashTable* table) {
    for(int i = 0; i < TABLE_SIZE; i++) {
        node* temp = table->head[i];
        while(temp) {
            node* prev = temp;
            temp = temp->next;
            free(prev);
        }
    }
    free(table);
}
int charger_dictionnaire(const char *nom_fichier, HashTable* dictionnaire) {
    FILE *file = fopen(nom_fichier, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1;
    }
    char mot[MAX_LONGUEUR_MOT];
    int index = 0;
    while (fscanf(file, "%s", mot) != EOF) {
        convertir_caracteres_speciaux(mot);
        insert(dictionnaire, mot, index);
        index++;
    }
    fclose(file);
    return index;
}
int verifier_presence_groupe_lettres(const char *groupe_lettres, HashTable* dictionnaire) {
    int presence = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        node* temp = dictionnaire->head[i];
        while(temp) {
            if (strstr(temp->key, groupe_lettres) != NULL) {
                presence++;
                if (presence >= 500) {
                    return 1;
                }
            }
            temp = temp->next;
        }
    }
    return 0;
}
void convertir_caracteres_speciaux(char *mot) {
for (size_t i = 0; i < strlen(mot); i++)
        {
            if (mot[i] < 0 && mot[i] != ' ')
            {
                // treat the special character
                if(mot[i] == -61){
                    switch(mot[i+1])
                    {
                        case -87:
                        case -86:
                        case -88:
                        case -85:
                            mot[i] = 'e';
                            break;
                        case -71:
                        case -68:
                        case -69:
                            mot[i] = 'u';
                            break;
                        case -92:
                        case -94:
                        case -96:
                            mot[i] = 'a';
                            break;

                        case -76:
                            mot[i] = 'o';
                            break;

                        case -81:
                        case -82:
                            mot[i] = 'i';
                            break;
                        case -89:
                            mot[i] = 'c';
                            break;
                        default:
                            printf("caractère spécial non traité\n");
                            continue;
                    }
                    // delete the second character
                    for (size_t j = i+1; j < strlen(mot); j++)
                    {
                        mot[j] = mot[j+1];
                    } 
                }
            }
        }
    }
node* lookup(HashTable* table, char* key) {
    // fonction de recherche qui renvoie le nœud correspondant à la clé donnée
    // Utilisez la fonction de hachage pour obtenir l'index pour la clé
    int hashIndex = hashFunction(key);

    // Parcourez la liste chaînée à cet index de la table de hachage
    node* temp = table->head[hashIndex];
    while (temp) {
        // Si la clé du nœud actuel correspond à la clé recherchée, renvoyez le nœud
        if (strcmp(temp->key, key) == 0) {
            return temp;
        }
        temp = temp->next;
    }

    // Si aucun nœud n'a été trouvé avec la clé recherchée, renvoyez NULL
    return NULL;
}

int generer_groupe_lettres(HashTable* groupe_lettres, HashTable* dictionnaire) {
    // Parcourir chaque entrée de la table de hachage du dictionnaire
    int index = 0;
    FILE *file = fopen("groupes_lettres.txt", "a");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1;
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        node* temp = dictionnaire->head[i];
        // Parcourir chaque nœud dans la liste chaînée à cet index
        while(temp) {
            int mot_length = strlen(temp->key);
            // Générer tous les groupes de lettres de 2 à 5 lettres
            for (int len = 2; len <= 5; len++) {
                for (int start = 0; start <= mot_length - len; start++) {
                    char groupe_lettres_temp[MAX_LONGUEUR_MOT] = {0};
                    strncpy(groupe_lettres_temp, &(temp->key[start]), len);
                    groupe_lettres_temp[len] = '\0'; // Assurez-vous que la chaîne est terminée
                    // Insérer le groupe de lettres dans la table de hachage
                    if (lookup(groupe_lettres, groupe_lettres_temp) == NULL) {
                        // Vérifier si le groupe de lettres existe déjà dans la table de hachage
                        if(verifier_presence_groupe_lettres(groupe_lettres_temp, dictionnaire)==1) {
                            insert(groupe_lettres, groupe_lettres_temp, index);
                            printf("Groupe de lettres ajouté: %s\n", groupe_lettres_temp);
                            index++;
                            // stocker les groupes de lettres dans un fichier
                            fprintf(file, "%s\n", groupe_lettres_temp);
                        }
                    }
                }
            }
            temp = temp->next;
        }
    }
    fclose(file);
    return index;
}


void generer_groupe_lettre_aleatoire(HashTable *groupe_lettres, char* groupe_lettre_aleatoire) {
    // Générer un groupe de lettres aléatoires à partir d'un hashTable de plusieurs groupes de lettres
    node* temp = NULL;
    int index;
    do {
        index = rand() % TABLE_SIZE;
        temp = groupe_lettres->head[index];
    } while(temp == NULL);

    while(temp) {
        strcpy(groupe_lettre_aleatoire, temp->key);
        temp = temp->next;
    }
}
void generer_groupe_lettre_aleatoire_v2(HashTable *dictionnaire, char* groupe_lettre_aleatoire) {
    // Compter le nombre total de clés dans la table de hachage
    int totalKeys = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        totalKeys += dictionnaire->count[i];
    }
    
    // Créer un tableau pour stocker toutes les clés
    char** allKeys = malloc(totalKeys * sizeof(char*));
    if (!allKeys) {
        printf("Erreur d'allocation de mémoire\n");
        return;
    }

    // Remplir le tableau avec toutes les clés de la table de hachage
    int index = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        node* temp = dictionnaire->head[i];
        while (temp) {
            allKeys[index] = temp->key;
            index++;
            temp = temp->next;
        }
    }
    do{
        // Sélectionner une clé aléatoire du tableau
        if (totalKeys > 0) {
            int randomIndex = rand() % totalKeys;
            char mot[MAX_LONGUEUR_MOT]; // Déclarez mot comme un tableau de caractères
            strcpy(mot, allKeys[randomIndex]);

            //choisir un groupe de lettres aléatoire dans le mot
            int mot_length = strlen(mot);
            int len = rand() % 4 + 2; // entre 2 et 5
            int start = rand() % (mot_length - len + 1);
            strncpy(groupe_lettre_aleatoire, &(mot[start]), len);
            groupe_lettre_aleatoire[len] = '\0'; // Assurez-vous que la chaîne est terminée
        } else {
            // Gérer le cas où totalKeys est zéro
            printf("Aucune clé dans le dictionnaire\n");
            return;
        }
    } while(verifier_presence_groupe_lettres(groupe_lettre_aleatoire, dictionnaire) == 0);

    // Libérer la mémoire allouée pour le tableau
    free(allKeys);
}
// Fonction pour vérifier si le groupe de lettres existe dans au moins 500 mots du dictionnaire
int verifier_presence_groupe_lettres_array(const char *groupe_lettres, char *dictionnaire[], int taille_dictionnaire) {
    int presence = 0;
    for (int i = 0; i < taille_dictionnaire; i++) {
        if (strstr(dictionnaire[i], groupe_lettres) != NULL) {
            presence++;
            if (presence >= 500) {
                break;
            }
        }
    }
    return presence >= 500;
}
void generer_groupe_lettres_array(char *groupe_lettres, char *dictionnaire[], int taille_dictionnaire, HashTable* groupe_lettres_utilisé) {
    // Générer un groupe de lettres aléatoires à partir d'un tableau de mots
    do{
        memset(groupe_lettres, 0, strlen(groupe_lettres));
        int index = rand() % taille_dictionnaire;
        char mot[MAX_LONGUEUR_MOT];
        convertir_caracteres_speciaux(dictionnaire[index]);
        strcpy(mot, dictionnaire[index]);
        int mot_length = strlen(mot);
        int len = rand() % 4 + 2; // entre 2 et 5
        int start = rand() % (mot_length - len + 1);
        strncpy(groupe_lettres, &(mot[start]), len);
        groupe_lettres[len] = '\0'; // Assurez-vous que la chaîne est terminée
    } while(verifier_presence_groupe_lettres_array(groupe_lettres, dictionnaire, taille_dictionnaire) == 0 || lookup(groupe_lettres_utilisé, groupe_lettres) != NULL);
}

int mot_existe(char *mot, HashTable* dictionnaire) {
    convertir_caracteres_speciaux(mot);
    node* temp = search(dictionnaire, mot);
    return temp != NULL;
}


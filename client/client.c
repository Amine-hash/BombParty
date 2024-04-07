#include "client.h"

int main() {
        sem_t *semaphore;
    // Ouverture du sémaphore partagé
    semaphore = sem_open(SEMAPHORE_NAME, O_RDWR); // si le sémaphore n'existe pas, il est créé avec une valeur initiale de 0 
    if (semaphore == SEM_FAILED) {
        perror("Erreur lors de l'ouverture du sémaphore");
        exit(EXIT_FAILURE);
    }
        // Récupération de l'identifiant de la mémoire partagée
    int shmid = shmget(SHARED_MEMORY_KEY, sizeof(pid_t) + MAX_PLAYERS * sizeof(ClientInfo), 0666);
    if (shmid == -1) {
        perror("Erreur lors de l'accès à la mémoire partagée");
        exit(EXIT_FAILURE);
    }
    printf("shmid = %d\n", shmid);
    // Attachement de la mémoire partagée
    char *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void *) -1) {
        perror("Erreur lors de l'attachement de la mémoire partagée");
        exit(EXIT_FAILURE);
    }

    // Récupération du PID du serveur depuis la mémoire partagée
    pid_t server_pid;
    memcpy(&server_pid, shm_ptr, sizeof(pid_t));
    printf("PID du serveur : %d\n", server_pid);
    // Envoi d'un signal au serveur pour indiquer la connexion
    server_pid = (pid_t) server_pid;
    if (kill(server_pid, SIGUSR1) == -1) {
        perror("Erreur lors de l'envoi du signal au serveur");
        exit(EXIT_FAILURE);
    }
    // Attente de la libération du sémaphore par le serveur
    if (sem_wait(semaphore) == -1) {
        perror("Erreur lors de l'attente du sémaphore");
        exit(EXIT_FAILURE);
    }
    // Attachement de la mémoire partagée
    ClientInfo client[MAX_PLAYERS];
    int client_pid=0;
    char tube_name[MAX_TUBE_NAME_LENGTH] = ""; // Initialisation du tube_name
    do
        {
            memcpy(&client, shm_ptr + sizeof(pid_t), MAX_PLAYERS*sizeof(ClientInfo));
            // Détachement de la mémoire partagée
            shmdt(shm_ptr);
            // Recherche du nom du tube nommé associé au PID dans la liste
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (client[i].pid == getpid()) {
                    client_pid = client[i].pid;
                    strcpy(tube_name, client[i].tube_name);
                    break;
                }
            }
        }
    while (client_pid != getpid());
    // Lecture du nom du tube nommé
    printf("Nom du tube nommé associé au client : %s\n", tube_name);
    // Ouverture du tube nommé
   
    // Fermeture du tube nommé

    // Communication avec le serveur via le tube nommé
    
    // Vous pouvez ouvrir le tube nommé ici et échanger des données avec le serveur

    //Create a menu for the client with the following options:
    //1. Create a new game
    //2. Join an existing game
    //3. Exit
    //If the client selects option 1, the client should send a message to the server to create a new game. The server should respond with the game ID.
    //If the client selects option 2, the client should send a message to the server to join an existing game. The server should respond with the game ID.
    //If the client selects option 3, the client should send a message to the server to exit the game.
    //The client should handle the responses from the server and display the appropriate messages to the user.
    //The client should continue to display the menu until the user selects option 3 to exit the game.
    //The client should close the tube and remove the tube name when the game is over.
    //The client should also detach the shared memory segment and close the semaphore before exiting.
    //The client should handle any errors that occur during the game and display appropriate error messages to the user.    
    int choice;
    char message[50];
    printf("1. Create a new game\n");
    printf("2. Join an existing game\n");
    printf("3. Exit\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    switch (choice)
    {
    case 1:
        strcpy(message, "Créer une nouvelle partie");
        break;
    case 2:
        strcpy(message, "Rejoindre une partie existante");
        break;
    case 3:
        strcpy(message, "Quitter le jeu");
        break;
    default:
        printf("Choix invalide\n");
        break;
    }
    int tube = open(tube_name, O_WRONLY);
    if (tube == -1) {
        perror("Erreur lors de l'ouverture du tube nommé");
        exit(EXIT_FAILURE);
    }
    // Envoi d'un message au serveur
    if (write(tube, message, sizeof(message)) == -1) {
        perror("Erreur lors de l'envoi du message au serveur");
        exit(EXIT_FAILURE);
    }
    close(tube);
    // Détachement de la mémoire partagée
    return 0;
}

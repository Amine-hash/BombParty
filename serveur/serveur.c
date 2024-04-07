#include "serveur.h"
#include <pthread.h>

sem_t *semaphore;
pid_t client_pids[MAX_PLAYERS];
int num_clients = 0;

void *client_thread(void *arg) {
    pid_t pid = *((pid_t *) arg);

    // Génération du nom du tube nommé
    char tube_name[MAX_TUBE_NAME_LENGTH];
    sprintf(tube_name, "/tmp/tube_%d", pid);

    // Création du tube nommé
    if (mkfifo(tube_name, 0666) == -1) {
        perror("Erreur lors de la création du tube nommé");
        exit(EXIT_FAILURE);
    }

    // Création de la mémoire partagée
    int shmid = shmget(SHARED_MEMORY_KEY, sizeof(pid_t) + MAX_PLAYERS * sizeof(ClientInfo), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Erreur lors de la création de la mémoire partagée");
        exit(EXIT_FAILURE);
    }

    // Attachement de la mémoire partagée
    char *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void *) -1) {
        perror("Erreur lors de l'attachement de la mémoire partagée");
        exit(EXIT_FAILURE);
    }

    // Recherche d'un emplacement libre pour stocker les informations du client
    long unsigned int offset = sizeof(pid_t);
    while (*(int *)(shm_ptr + offset) != 0 && offset < MAX_PLAYERS * sizeof(ClientInfo)) {
        offset += sizeof(ClientInfo);
    }
    if (offset >= MAX_PLAYERS * sizeof(ClientInfo)) {
        perror("Mémoire partagée pleine");
        exit(EXIT_FAILURE);
    }

    // Associer pid et tube_name
    ClientInfo client;
    client.pid = pid;
    strcpy(client.tube_name, tube_name);
    memcpy(shm_ptr + offset, &client, sizeof(ClientInfo));
    
    // Détachement de la mémoire partagée
    if (shmdt(shm_ptr) == -1) {
        perror("Erreur lors du détachement de la mémoire partagée");
        exit(EXIT_FAILURE);
    }

    // Libération du sémaphore pour permettre au client d'accéder à nouveau à la mémoire partagée
    if (sem_post(semaphore) == -1) {
        perror("Erreur lors de la libération du sémaphore");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        // lit le tube nommé
        printf("Attente de message...\n");
        int tube = open(tube_name, O_RDONLY);
        if (tube == -1) {
            perror("Erreur lors de l'ouverture du tube nommé");
            exit(EXIT_FAILURE);
        }
        char buffer[256];
        if (read(tube, buffer, 256) == -1) {
            perror("Erreur lors de la lecture du tube nommé");
            exit(EXIT_FAILURE);
        }
        printf("Message reçu : %s\n", buffer);
        close(tube);
    }
    
    pthread_exit(NULL);
}

void signal_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGUSR1) {
        pid_t pid = info->si_pid;

        if (num_clients < MAX_PLAYERS) {
            client_pids[num_clients++] = pid;

            pthread_t thread;
            if (pthread_create(&thread, NULL, client_thread, &pid) != 0) {
                perror("Erreur lors de la création du thread client");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Nombre maximum de clients atteint. Ignorer le signal SIGUSR1.\n");
        }
    }
    else if (sig == SIGINT)
    {
        printf("Arrêt du serveur\n");
        // Suppression du sémaphore
        if (sem_unlink(SEMAPHORE_NAME) == -1) 
            perror("Erreur lors de la suppression du sémaphore");
        // stopper les threads
        for (int i = 0; i < num_clients; i++) {
            kill(client_pids[i], SIGINT);
        }
        // supprimer et vider la mémoire partagée
        int shmid = shmget(SHARED_MEMORY_KEY, sizeof(pid_t) + MAX_PLAYERS * sizeof(ClientInfo), 0666);
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("Erreur lors de la suppression de la mémoire partagée");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else {
        printf("Signal inconnu reçu\n");
    }
}

int main() {
    printf("Serveur lancé\n");
    // Ouverture du sémaphore
    semaphore = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 0); // Création du sémaphore avec une valeur initiale de 0 pour bloquer le client
    if (semaphore == SEM_FAILED) {
        perror("Erreur lors de l'ouverture du sémaphore");
        exit(EXIT_FAILURE);
    }

    // Installation du gestionnaire de signal
    struct sigaction action;
    action.sa_sigaction = signal_handler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGUSR1, &action, NULL) == -1) {
        perror("Erreur lors de l'installation du gestionnaire de signal");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &action, NULL))
    {
        perror("Erreur lors de l'installation du gestionnaire de signal");
        exit(EXIT_FAILURE);
    }
    

    // Création de la mémoire partagée pour stocker les PIDs des clients
    int shmid = shmget(SHARED_MEMORY_KEY, sizeof(pid_t) + MAX_PLAYERS * sizeof(ClientInfo), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Erreur lors de la création de la mémoire partagée");
        exit(EXIT_FAILURE);
    }

    // Attachement de la mémoire partagée
    char *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void *) -1) {
        perror("Erreur lors de l'attachement de la mémoire partagée");
        exit(EXIT_FAILURE);
    }

    // Ajout du pid du serveur dans la mémoire partagée
    pid_t server_pid = getpid();
    memcpy(shm_ptr, &server_pid, sizeof(pid_t));

    // Attente des signaux
    while (1) {
        pause();
    }

    // Détachement de la mémoire partagée
    if (shmdt(shm_ptr) == -1) {
        perror("Erreur lors du détachement de la mémoire partagée");
        exit(EXIT_FAILURE);
    }
    // Suppression du sémaphore
    if (sem_unlink(SEMAPHORE_NAME) == -1) 
        perror("Erreur lors de la suppression du sémaphore");
    return 0;
}

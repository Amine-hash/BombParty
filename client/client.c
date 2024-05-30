#include "client.h"
int tube_fd;
char tube_name[MAX_TUBE_NAME_LENGTH];
int FLAG_DEMARRAGE_PARTIE = 0;
void create_game(int msgid, pid_t pid) {
    char use_defaults;
    Message message;
    message.type = 1;
    message.partie.id = rand();
    message.partie.nombre_joueur_courant = 1; // Le créateur de la partie est le premier joueur
    message.partie.joueurs[0].id = pid;
    message.partie.joueurs[0].nombre_vie = DEFAULT_VIES;
    message.partie.joueurs[0].etat = EN_ATTENTE;
    strcpy(message.partie.joueurs[0].tube_name, tube_name);
    printf("Entrez votre pseudo: ");
    scanf("%s", message.partie.joueurs[0].pseudo);

    printf("Voulez-vous créer une partie par défaut ? (y/n): ");
    scanf(" %c", &use_defaults);

    if (use_defaults == 'y') {
        message.partie.nombre_vies = DEFAULT_VIES;
        message.partie.nombre_joueur_max = DEFAULT_JOUEURS;
    } else {
        printf("Entrez le nombre de vies : ");
        scanf("%d", &message.partie.nombre_vies);
        printf("Entrez le nombre de joueurs : ");
        scanf("%d", &message.partie.nombre_joueur_max);
    }

    CHECK(msgsnd(msgid, &message, sizeof(Partie), 0), -1, "Erreur lors de l'envoi du message à la BAL");

    Partie *partie = malloc(sizeof(Partie));
    if (partie == NULL) {
        perror("Erreur lors de l'allocation mémoire");
        exit(EXIT_FAILURE);
    }
    *partie = message.partie;
    char message_serv[50];
    snprintf(message_serv, sizeof(message), "NewGame|%d", msgid);
    CHECK(write(tube_fd, message_serv, sizeof(message_serv)), -1, "Erreur lors de l'envoi du message au serveur");   
    close(tube_fd);
    pthread_t game_thread;
    pthread_create(&game_thread, NULL, client_game_handler, partie);
    pthread_join(game_thread, NULL);
}

void join_game() {
    // Ouverture du sémaphore
    sem_t *semaphore = sem_open(SEMAPHORE_NAME, O_RDWR);
    CHECK(semaphore, SEM_FAILED, "Erreur lors de l'ouverture du sémaphore");

    // Lecture de la mémoire partagée
    int shmid = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, 0666);
    CHECK(shmid, -1, "Erreur lors de la récupération de la mémoire partagée");

    char *shm_ptr = shmat(shmid, NULL, 0);
    CHECK(shm_ptr, (void *)-1, "Erreur lors de l'attachement de la mémoire partagée");

    int *num_parties = (int *)(shm_ptr + sizeof(pid_t));
    Partie *parties = (Partie *)(shm_ptr + sizeof(pid_t) + sizeof(int));
    if (*num_parties == 0) {
        printf("Aucune partie disponible.\n");
        shmdt(shm_ptr);
        return;
    }
    int num_parties_disponibles = 0;
    printf("Parties disponibles:\n");
    for (int i = 0; i < *num_parties; i++) {
        if(parties[i].etat == PARTIE_EN_ATTENTE_JOUEURS){
            printf("ID: %d, Joueurs: %d/%d\n", parties[i].id, parties[i].nombre_joueur_courant, parties[i].nombre_joueur_max);
            num_parties_disponibles++;
        }
    }
    if (num_parties_disponibles == 0) {
        printf("Aucune partie disponible.\n");
        shmdt(shm_ptr);
        return;
    }
    int partie_id;
    char pseudo[MAX_NAME_LENGTH];
    printf("Entrez l'ID de la partie à rejoindre: ");
    scanf("%d", &partie_id);
    printf("Entrez votre pseudo: ");
    scanf("%s", pseudo);
    // Envoyer une demande de rejoindre une partie au serveur
    char message[256];
    snprintf(message, sizeof(message), "JoinGame|%d|%s", partie_id, pseudo);
    write(tube_fd, message, strlen(message) + 1);
    close(tube_fd);
    sem_post(semaphore);
    shmdt(shm_ptr);
    // Attendre une confirmation ou une erreur du serveur
    int confirm_tube = open(tube_name, O_RDONLY);
    CHECK(confirm_tube, -1, "Erreur lors de l'ouverture du tube nommé pour confirmation");
    char confirm_buffer[256];
    ssize_t bytes_read = read(confirm_tube, confirm_buffer, sizeof(confirm_buffer));
    if (bytes_read > 0) {
         close(confirm_tube);
        if (strncmp(confirm_buffer, "JoinConfirm", 11) == 0) {
            printf("Rejoint la partie avec succès.\n");
            Partie *partie_rejointe = recuperer_infos_partie(partie_id);
            // Here you might want to get the updated details about the joined party
            pthread_t game_thread;
            pthread_create(&game_thread, NULL, client_game_handler, partie_rejointe);
            pthread_join(game_thread, NULL);
        } else if (strncmp(confirm_buffer, "JoinError", 9) == 0) {
            printf("Erreur: %s\n", confirm_buffer + 10);
        }
    }
    exit(EXIT_SUCCESS);
}
void *client_game_handler(void *arg) {
    Partie *partie = (Partie *)arg;
    printf("Partie en attente de nouveaux joueurs.\n");

    char buffer[256];
    int tube_fd = open(tube_name, O_RDONLY);
    while (FLAG_DEMARRAGE_PARTIE == 0) {
        if (tube_fd == -1) {
            perror("Erreur lors de l'ouverture du tube nommé");
            continue;
        }

        ssize_t bytes_read = read(tube_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the buffer
            if (strncmp(buffer, "PlayerJoin", 10) == 0) {
                printf("Le joueur %s a rejoint la partie.\n", buffer + 11);
            }
        } else if (bytes_read == 0) {
            // Si la lecture retourne 0, cela signifie que l'extrémité d'écriture a été fermée
            printf("Le serveur a fermé le tube nommé.\n");
            continue;
        } else {
            if (bytes_read == -1) {
                perror("Erreur lors de la lecture du tube nommé");
            }
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        printf("Attente de nouveaux joueurs...\n");
    }
    close(tube_fd);
    return NULL;
}



void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        printf("Signal SIGUSR1 reçu.\n");
        FLAG_DEMARRAGE_PARTIE = 1;
    }
}
int main() {
    sem_t *semaphore;
        
    // Installation du gestionnaire de signal
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    CHECK(sigaction(SIGUSR1, &action, NULL), -1, "Erreur lors de l'installation du gestionnaire de signal de SIGUSR1");
    // Ouverture du sémaphore partagé
    semaphore = sem_open(SEMAPHORE_NAME, O_RDWR); // si le sémaphore n'existe pas, il est créé avec une valeur initiale de 0 
    CHECK(semaphore, SEM_FAILED, "Erreur lors de l'ouverture du sémaphore");
        // Récupération de l'identifiant de la mémoire partagée
    int shmid = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, 0666);
    CHECK(shmid, -1, "Erreur lors de la récupération de la mémoire partagée");
    printf("shmid = %d\n", shmid);
    // Attachement de la mémoire partagée
    char *shm_ptr = shmat(shmid, NULL, 0);
    CHECK(shm_ptr, (char *) -1, "Erreur lors de l'attachement de la mémoire partagée");

    // Récupération du PID du serveur depuis la mémoire partagée
    pid_t server_pid;
    memcpy(&server_pid, shm_ptr, sizeof(pid_t));
    printf("PID du serveur : %d\n", server_pid);
    int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    CHECK(msgid, -1, "Erreur lors de la création de la BAL");
    // Envoi d'un signal au serveur pour indiquer la connexion
    server_pid = (pid_t) server_pid;
    CHECK(kill(server_pid, SIGUSR1), -1, "Erreur lors de l'envoi du signal au serveur");
    // Attente de la libération du sémaphore par le serveur
    CHECK(sem_wait(semaphore), -1, "Erreur lors de l'attente de la libération du sémaphore");
    sprintf(tube_name,"%s%d", PIPE_PATH, getpid());

    // Lecture du nom du tube nommé
    printf("Nom du tube nommé associé au client : %s\n", tube_name);
    
    int choice;
    do{
    printf("1. Create a new game\n");
    printf("2. Join an existing game\n");
    printf("3. Exit\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    printf("tube_name = %s\n", tube_name);
    tube_fd = open(tube_name, O_WRONLY);
    CHECK(tube_fd, -1, "Erreur lors de l'ouverture du tube nommé");
    switch (choice)
    {
    case 1:
        create_game(msgid, getpid());
        break;
    case 2:
        join_game(server_pid);
        break;
    case 3:
        printf("Quitter le jeu\n");
        break;
    default:
        printf("Choix invalide\n");
        break;
    }
    }while(choice != 3);
    close(tube_fd);
    // Détachement de la mémoire partagée
    CHECK(shmdt(shm_ptr), -1, "Erreur lors du détachement de la mémoire partagée");
    return 0;
}
Partie *recuperer_infos_partie(int partie_id) {
    // Ouverture du sémaphore
    sem_t *semaphore = sem_open(SEMAPHORE_NAME, O_RDWR);
    CHECK(semaphore, SEM_FAILED, "Erreur lors de l'ouverture du sémaphore");

    // Lecture de la mémoire partagée
    int shmid = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, 0666);
    CHECK(shmid, -1, "Erreur lors de la récupération de la mémoire partagée");

    char *shm_ptr = shmat(shmid, NULL, 0);
    CHECK(shm_ptr, (void *)-1, "Erreur lors de l'attachement de la mémoire partagée");

    int *num_parties = (int *)(shm_ptr + sizeof(pid_t));
    Partie *parties = (Partie *)(shm_ptr + sizeof(pid_t) + sizeof(int));

    Partie *partie_rejointe = NULL;
    for (int i = 0; i < *num_parties; i++) {
        if (parties[i].id == partie_id) {
            partie_rejointe = &parties[i];
            break;
        }
    }
    if (partie_rejointe == NULL) {
        printf("Erreur: Impossible de trouver la partie dans la mémoire partagée.\n");
        shmdt(shm_ptr);
        return NULL;
    }

    shmdt(shm_ptr);
    sem_close(semaphore);
    return partie_rejointe;
}

#include "client.h"

int tube_fd;
char tube_name[MAX_TUBE_NAME_LENGTH];
volatile sig_atomic_t FLAG_DEMARRAGE_PARTIE = 0;
volatile sig_atomic_t FLAG_TOUR_JOUEUR = 0;
volatile sig_atomic_t FLAG_TIMEOUT = 0;
volatile sig_atomic_t FLAG_SERVEUR_WORKING = 0;
volatile sig_atomic_t FLAG_TIMEOUT_SRV = 0;
volatile sig_atomic_t FLAG_PARTIE = 1;
void create_game(int msgid, pid_t pid) {
    char use_defaults;
    Message message;
    message.type = 1;
    message.partie.id = rand();
    message.partie.nombre_joueur_courant = 1; // Le créateur de la partie est le premier joueur
    message.partie.joueurs[0].id = pid;
    message.partie.joueurs[0].etat = EN_ATTENTE;
    strcpy(message.partie.joueurs[0].tube_name, tube_name);
    printf("Entrez votre pseudo: ");
    scanf("%s", message.partie.joueurs[0].pseudo);

    printf("Voulez-vous créer une partie par défaut ? (y/n): ");
    scanf(" %c", &use_defaults);

    if (use_defaults == 'y') {
        message.partie.nombre_vies = DEFAULT_VIES;
        message.partie.nombre_joueur_max = 2;
    } else {
        printf("Entrez le nombre de vies : ");
        scanf("%d", &message.partie.nombre_vies);
        printf("Entrez le nombre de joueurs : ");
        scanf("%d", &message.partie.nombre_joueur_max);
    }
    message.partie.joueurs[0].nombre_vie = message.partie.nombre_vies;
    CHECK(msgsnd(msgid, &message, sizeof(Partie), 0), -1, "Erreur lors de l'envoi du message à la BAL");

    Partie *partie = malloc(sizeof(Partie));
    if (partie == NULL) {
        perror("Erreur lors de l'allocation mémoire");
        exit(EXIT_FAILURE);
    }
    *partie = message.partie;

    char message_serv[50];
    snprintf(message_serv, sizeof(message_serv), "NewGame|%d", msgid);
    CHECK(write_to_pipe(tube_fd, message_serv, sizeof(message_serv)), -1, "Erreur lors de l'envoi du message au serveur");

    close_pipe(tube_fd);

    pthread_t game_thread;
    message.partie.joueurs[0].etat = EN_JEU;
    pthread_create(&game_thread, NULL, client_game_handler, partie);
    pthread_join(game_thread, NULL);
    exit(EXIT_SUCCESS);
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
    write_to_pipe(tube_fd, message, strlen(message) + 1);
    close_pipe(tube_fd);

    sem_post(semaphore);
    shmdt(shm_ptr);

    // Attendre une confirmation ou une erreur du serveur
    int confirm_tube = open_named_pipe(tube_name, O_RDONLY);
    CHECK(confirm_tube, -1, "Erreur lors de l'ouverture du tube nommé pour confirmation");

    char confirm_buffer[256];
    ssize_t bytes_read = read_from_pipe(confirm_tube, confirm_buffer, sizeof(confirm_buffer));
    if (bytes_read > 0) {
        close_pipe(confirm_tube);
        if (strncmp(confirm_buffer, "JoinConfirm", 11) == 0) {
            printf("Rejoint la partie avec succès.\n");
            Partie *partie_rejointe = recuperer_infos_partie(partie_id);
            pthread_t game_thread;
            pthread_create(&game_thread, NULL, client_game_handler, partie_rejointe);
            pthread_join(game_thread, NULL);
        } else if (strncmp(confirm_buffer, "JoinError", 9) == 0) {
            printf("Erreur: %s\n", confirm_buffer + 10);
        }
    }
    exit(EXIT_SUCCESS);
}
void timeout_handler(int signum) {
    if (signum == SIGALRM) {
        if(FLAG_SERVEUR_WORKING == 0)
            FLAG_TIMEOUT = 1;
        else
            FLAG_TIMEOUT_SRV = 1;
        printf("\n");
        printf("Appuyer sur entrée\n");
    }
    if (signum == SIGRTMIN) {
        printf("Vous avez gagnez la partie\n");
        FLAG_PARTIE = 0;
    }
}
void read_input(char *buffer, size_t size) {
    ssize_t bytes_read;
    do {
        bytes_read = read(STDIN_FILENO, buffer, size - 1);
        if (bytes_read == -1) {
            if (errno == EINTR) {
                // Lecture interrompue par un signal, réessayer
                printf("Lecture interrompue par un signal, réessayez\n");
                break;
            } else {
                perror("Erreur lors de la lecture de l'entrée");
                exit(EXIT_FAILURE);
            }
        }
        buffer[bytes_read] = '\0';
    } while (bytes_read == -1);
    fflush(stdout);
}
void start_timer() {
    struct itimerval timer;
    timer.it_value.tv_sec = 10;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
}

void stop_timer() {
    struct itimerval timer = { {0, 0}, {0, 0} };
    setitimer(ITIMER_REAL, &timer, NULL);
}
void *client_game_handler(void *arg) {
    Partie *partie = (Partie *)arg;
    struct sigaction action;
    action.sa_handler = timeout_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);
    sigaction(SIGRTMIN, &action, NULL);
    // créer un nom de semaphore unique à la partie à l'aide du pid du premier joueur
    char semaphore_name[50];
    char semaphore_name2[50];
    char semaphore_name3[50];
    snprintf(semaphore_name, sizeof(semaphore_name), "%s_%d", SEMAPHORE_NAME, partie->joueurs[0].id);
    snprintf(semaphore_name2, sizeof(semaphore_name2), "%s_%d_mot", SEMAPHORE_NAME, partie->joueurs[0].id);
    snprintf(semaphore_name3, sizeof(semaphore_name3), "%s_%d_reponse", SEMAPHORE_NAME, partie->joueurs[0].id);
    printf("Partie en attente de nouveaux joueurs.\n");
    ssize_t bytes_read;
    char buffer[256];

    // Ouverture du tube nommé en mode non-bloquant
    int tube_fd = open_named_pipe(tube_name, O_RDONLY | O_NONBLOCK);
    if (tube_fd == -1) {
        perror("Erreur lors de l'ouverture du tube nommé");
        pthread_exit(NULL);
    }

    while (FLAG_DEMARRAGE_PARTIE == 0) {
        bytes_read = read_from_pipe(tube_fd, buffer, sizeof(buffer) - 1);  // -1 pour laisser de l'espace pour le null terminator
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if (strncmp(buffer, "PlayerJoin", 10) == 0) {
                printf("Le joueur %s a rejoint la partie.\n", buffer + 11);
            }
        } else if (bytes_read == -1 && errno != EAGAIN) {
            perror("Erreur lors de la lecture dans le tube nommé");
            close_pipe(tube_fd);
            pthread_exit(NULL);
        }
    }
    close_pipe(tube_fd);
    sem_t *semaphore_partie = sem_open(semaphore_name, O_CREAT, 0666, 0);
    if(semaphore_partie == SEM_FAILED){
        perror("Erreur lors de la création du sémaphore");
        pthread_exit(NULL);
    }
    sem_t *semaphore_mot = sem_open(semaphore_name2, O_CREAT, 0666, 0);
    if (semaphore_partie == SEM_FAILED) {
        perror("Erreur lors de la création du sémaphore");
        pthread_exit(NULL);
    }
    sem_t *semaphore_reponse = sem_open(semaphore_name3, O_CREAT, 0666, 0);
    if (semaphore_partie == SEM_FAILED) {
        perror("Erreur lors de la création du sémaphore");
        pthread_exit(NULL);
    }    

    for (int i = 3; i >= 0; i--) {
        printf("%d.....", i);
        fflush(stdout);
        sleep(1);
    }
    printf("\n");
    while (FLAG_PARTIE) {
        tube_fd = open_named_pipe(tube_name, O_RDWR | O_NONBLOCK);
        if (tube_fd == -1) {
            perror("Erreur lors de l'ouverture du tube nommé pour le groupe de lettres");
            sem_close(semaphore_partie);
            pthread_exit(NULL);
        }

        char groupe_lettres_client[10];
        while ((bytes_read = read_from_pipe(tube_fd, groupe_lettres_client, sizeof(groupe_lettres_client) - 1)) <= 0 && FLAG_PARTIE == 1);
        if(FLAG_PARTIE == 0){
            close_pipe(tube_fd);
            pthread_exit(NULL);
        }
        sem_post(semaphore_partie); // Signal after reading
        groupe_lettres_client[bytes_read] = '\0'; // Null-terminate the buffer
        printf("Groupe de lettres : %s\n", groupe_lettres_client);
        usleep(100000);
        if(FLAG_TOUR_JOUEUR == 0){
            close_pipe(tube_fd);
            continue;
        }
        char reponse[256];
        int mot_correct = 0;

        do {
        FLAG_TIMEOUT = 0;
        start_timer();
        char buffer2[256];
        printf("Entrez un mot: ");
        fflush(stdout);
        read_input(buffer2, sizeof(buffer2));
        sscanf(buffer2, "%s", buffer2);
        fflush(stdout);
        if(FLAG_TIMEOUT){
            stop_timer();
            printf("Temps écoulé !!!! Vous perdez une vie.\n");
            break;
        }
        FLAG_SERVEUR_WORKING = 1;
        write_to_pipe(tube_fd, buffer2, strlen(buffer2) + 1);
        sem_post(semaphore_partie); // Signal after writing
        sem_wait(semaphore_reponse); // Wait for the response
        while ((bytes_read = read_from_pipe(tube_fd, reponse, sizeof(reponse) - 1)) <= 0);
        if (bytes_read > 0) {
            reponse[bytes_read] = '\0'; // Null-terminate the buffer
            if (strncmp(reponse, "Correct", 7) == 0) {
                printf("Mot correct\n");
                mot_correct = 1;
                stop_timer();   
            } else {
                printf("Mot incorrect\n");
                printf("Groupe de lettres : %s\n", groupe_lettres_client);
                if(FLAG_TIMEOUT_SRV){
                    FLAG_TIMEOUT = 1;
                }
            }
        } else {
            perror("Erreur lors de la lecture de la réponse");
        }
        FLAG_SERVEUR_WORKING = 0;
        FLAG_TIMEOUT_SRV = 0;
    } while (!mot_correct);
    if (FLAG_TIMEOUT) {
            //trouver le joueur
            char buffer3[256];
            for(int i = 0; i < partie->nombre_joueur_courant; i++){
                if(partie->joueurs[i].id == getpid()){
                    //afficher info joueur
                    partie->joueurs[i].nombre_vie -= 1;
                    if (partie->joueurs[i].nombre_vie <= 0) {
                        printf("Vous avez perdu toutes vos vies !\n");
                        partie->joueurs[i].etat = PERDU;
                        // Envoyer un message au serveur pour indiquer la perte de toutes les vies
                        snprintf(buffer3, sizeof(buffer3), "PlayerOut|%d", partie->id);
                        write_to_pipe(tube_fd, buffer3, strlen(buffer3) + 1);
                        sem_post(semaphore_partie);
                        sem_wait(semaphore_reponse);
                        read_from_pipe(tube_fd, buffer3, sizeof(buffer3) - 1);
                        FLAG_PARTIE = 0;
                        sem_post(semaphore_mot);
                        pthread_exit(NULL);
                    } else {
                        printf("Vous avez perdu une vie !\n");
                        printf("Vies restantes : %d\n", partie->joueurs[i].nombre_vie);
                        snprintf(buffer3, sizeof(buffer3), "PlayerLostLife|%d", partie->id);
                        write_to_pipe(tube_fd, buffer3, strlen(buffer3) + 1);
                        sem_post(semaphore_partie);
                        sem_wait(semaphore_reponse);
                        read_from_pipe(tube_fd, buffer3, sizeof(buffer3) - 1);
                    }
                    break;
                }
            }
        }
        sem_post(semaphore_mot); // Signal after reading
        FLAG_TOUR_JOUEUR = 0;
    }
    close_pipe(tube_fd);
    sem_close(semaphore_partie);
    return NULL;
}

void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        printf("La partie va commencer.\n");
        FLAG_DEMARRAGE_PARTIE = 1;
    }
    if (signum == SIGUSR2) {
        printf("C'est à votre tour de jouer.\n");
        FLAG_TOUR_JOUEUR = 1;
    }
}

int main() {
    sem_t *semaphore;

    // Installation du gestionnaire de signal
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    CHECK(sigaction(SIGUSR1, &action, NULL), -1, "Erreur lors de l'installation du gestionnaire de signal de SIGUSR1");
    CHECK(sigaction(SIGUSR2, &action, NULL), -1, "Erreur lors de l'installation du gestionnaire de signal de SIGUSR2");

    // Ouverture du sémaphore partagé
    semaphore = sem_open(SEMAPHORE_NAME, O_RDWR); // si le sémaphore n'existe pas, il est créé avec une valeur initiale de 0
    CHECK(semaphore, SEM_FAILED, "Erreur lors de l'ouverture du sémaphore");

    // Récupération de l'identifiant de la mémoire partagée
    int shmid = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, 0666);
    CHECK(shmid, -1, "Erreur lors de la récupération de la mémoire partagée");
    printf("shmid = %d\n", shmid);

    // Attachement de la mémoire partagée
    char *shm_ptr = shmat(shmid, NULL, 0);
    CHECK(shm_ptr, (char *)-1, "Erreur lors de l'attachement de la mémoire partagée");

    // Récupération du PID du serveur depuis la mémoire partagée
    pid_t server_pid;
    memcpy(&server_pid, shm_ptr, sizeof(pid_t));
    printf("PID du serveur : %d\n", server_pid);

    int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    CHECK(msgid, -1, "Erreur lors de la création de la BAL");

    // Envoi d'un signal au serveur pour indiquer la connexion
    CHECK(kill(server_pid, SIGUSR1), -1, "Erreur lors de l'envoi du signal au serveur");

    // Attente de la libération du sémaphore par le serveur
    CHECK(sem_wait(semaphore), -1, "Erreur lors de l'attente de la libération du sémaphore");

    snprintf(tube_name, sizeof(tube_name), "%s%d", PIPE_PATH, getpid());

    // Lecture du nom du tube nommé
    printf("Nom du tube nommé associé au client : %s\n", tube_name);

    int choice;
    do {
        printf("1. Create a new game\n");
        printf("2. Join an existing game\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        printf("tube_name = %s\n", tube_name);
        tube_fd = open_named_pipe(tube_name, O_WRONLY);
        CHECK(tube_fd, -1, "Erreur lors de l'ouverture du tube nommé");

        switch (choice) {
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
    } while (choice != 3);

    close_pipe(tube_fd);

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
    // Afficher les informations de la partie
    printf("Informations de la partie:\n");
    if (partie_rejointe == NULL) {
        printf("Erreur: Impossible de trouver la partie dans la mémoire partagée.\n");
        shmdt(shm_ptr);
        return NULL;
    }

    sem_close(semaphore);
    return partie_rejointe;
}
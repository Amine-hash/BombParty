#include "serveur.h"

sem_t *semaphore;
pthread_t threads[MAX_PLAYERS];
pid_t client_pids[MAX_PLAYERS];
int num_clients = 0;
int num_parties = 0;
char *shm_ptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t notify_mutex = PTHREAD_MUTEX_INITIALIZER;

void create_named_pipe(const char *tube_name) {
    if (access(tube_name, F_OK) == 0) {
        CHECK(unlink(tube_name), -1, "Erreur lors de la suppression de l'ancien tube nommé");
    }
    CHECK(mkfifo(tube_name, 0666), -1, "Erreur lors de la création du tube nommé");
}

void send_confirmation(const char *tube_name, const char *message) {
    int confirm_tube = open(tube_name, O_WRONLY);
    CHECK(confirm_tube, -1, "Erreur lors de l'ouverture du tube nommé pour confirmation");
    write(confirm_tube, message, strlen(message) + 1);
    close(confirm_tube);
}

void handle_new_game(char *buffer, const char *tube_name) {
    int msgid;
    sscanf(buffer, "NewGame|%d", &msgid);

    Message message;
    CHECK(msgrcv(msgid, &message, sizeof(Partie), 1, 0), -1, "Erreur lors de la lecture de la BAL");

    pthread_mutex_lock(&mutex);
    if (num_parties < MAX_GAMES) {
        message.partie.id = num_parties + 1;
        message.partie.etat = PARTIE_EN_ATTENTE_JOUEURS;
        parties[num_parties] = message.partie;
        num_parties++;
        printf("Nouvelle partie créée avec l'ID %d.\n", message.partie.id);
        printf("Nombre de parties : %d\n", num_parties);
        printf("Informations de la partie : %d %d %d %d\n", message.partie.id, message.partie.nombre_joueur_courant, message.partie.nombre_joueur_max, message.partie.nombre_vies);

        memcpy(shm_ptr + sizeof(pid_t), &num_parties, sizeof(int));
        memcpy(shm_ptr + sizeof(pid_t) + sizeof(int), parties, num_parties * sizeof(Partie));

        notify_players(&message.partie, message.partie.joueurs[0].pseudo);
    } else {
        printf("Nombre maximum de parties atteint.\n");
    }
    pthread_mutex_unlock(&mutex);

    CHECK(msgctl(msgid, IPC_RMID, NULL), -1, "Erreur lors de la suppression de la BAL");
}

void notify_players(Partie *partie, const char *pseudo) {
    pthread_mutex_lock(&notify_mutex);
    for (int j = 0; j < partie->nombre_joueur_courant; j++) {
        printf("Envoi d'une notification de joueur à %s.\n", partie->joueurs[j].pseudo);
        int player_tube = open(partie->joueurs[j].tube_name, O_WRONLY);
        printf("tube_name : %s\n", partie->joueurs[j].tube_name);
        printf("player_tube : %d\n", player_tube);
        if (player_tube != -1) {
            char player_message[256];
            snprintf(player_message, sizeof(player_message), "PlayerJoin|%s", pseudo);
            if (write(player_tube, player_message, strlen(player_message) + 1) == -1) {
                perror("Erreur lors de l'écriture dans le tube nommé");
            }
            sleep(1); // attendre 1 seconde pour permettre au client de lire le message
            close(player_tube);
        } else {
            printf("Erreur lors de l'ouverture du tube nommé pour notification de joueur (%s).\n", partie->joueurs[j].tube_name);
        }
    }
    pthread_mutex_unlock(&notify_mutex);nice
}


void handle_join_game(char *buffer, const char *tube_name, pid_t pid) {
    int partie_id;
    char pseudo[MAX_NAME_LENGTH];
    sscanf(buffer, "JoinGame|%d|%s", &partie_id, pseudo);

    pthread_mutex_lock(&mutex);
    int partie_trouvee = 0;
    for (int i = 0; i < num_parties; i++) {
        if (parties[i].id == partie_id) {
            Partie *partie = &parties[i];
            strcpy(parties[i].joueurs[partie->nombre_joueur_courant].tube_name, tube_name);
            if (partie->nombre_joueur_courant < partie->nombre_joueur_max) {
                partie->joueurs[partie->nombre_joueur_courant].id = pid;
                strcpy(partie->joueurs[partie->nombre_joueur_courant].pseudo, pseudo);
                partie->nombre_joueur_courant++;
                printf("Le joueur %s a rejoint la partie %d.\n", pseudo, partie_id);
                memcpy(shm_ptr + sizeof(pid_t) + sizeof(int), parties, num_parties * sizeof(Partie));

                char confirm_message[256];
                snprintf(confirm_message, sizeof(confirm_message), "JoinConfirm|%d|%s", partie_id, pseudo);
                send_confirmation(tube_name, confirm_message);

                notify_players(partie, pseudo);

                if (partie->nombre_joueur_courant == partie->nombre_joueur_max) {
                    partie->etat = PARTIE_DEMARREE;
                    memcpy(shm_ptr + sizeof(pid_t) + sizeof(int), parties, num_parties * sizeof(Partie));
                    pthread_t thread;
                    CHECK(pthread_create(&thread, NULL, game_thread, partie), -1, "Erreur lors de la création du thread de la partie");
                }
                partie_trouvee = 1;
            } else {
                printf("La partie %d est pleine.\n", partie_id);
            }
            break;
        }
    }
    if (!partie_trouvee) {
        printf("La partie avec l'ID %d n'existe pas.\n", partie_id);

        char error_message[256];
        snprintf(error_message, sizeof(error_message), "JoinError|%d|%s", partie_id, "ID incorrect ou partie pleine");
        send_confirmation(tube_name, error_message);
    }
    pthread_mutex_unlock(&mutex);
}

void *client_thread(void *arg) {
    pid_t pid = *((pid_t *)arg);
    char tube_name[MAX_TUBE_NAME_LENGTH];
    sprintf(tube_name, "%s%d", PIPE_PATH, pid);

    create_named_pipe(tube_name);

    CHECK(sem_post(semaphore), -1, "Erreur lors de la libération du sémaphore");

    while (1) {
        int tube = open(tube_name, O_RDONLY); 
        CHECK(tube, -1, "Erreur lors de l'ouverture du tube nommé");

        char buffer[256];
        ssize_t bytes_read = read(tube, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            if (strncmp(buffer, "NewGame", 7) == 0) {
                handle_new_game(buffer, tube_name);
            } else if (strncmp(buffer, "JoinGame", 8) == 0) {
                handle_join_game(buffer, tube_name, pid);
            }
        }
        close(tube);
    }
    return NULL;
}
void *game_thread(void *arg) {
    Partie *partie = (Partie *) arg;
    printf("Thread de la partie %d démarré.\n", partie->id);
    // Gérer la logique du jeu ici...
    // envoyer un signal SIGUSR1 à chaque joueur pour commencer le jeu
    for (int i = 0; i < partie->nombre_joueur_courant; i++) {
        printf("Envoi d'un signal SIGUSR1 au joueur %d.\n", partie->joueurs[i].id);
        kill(partie->joueurs[i].id, SIGUSR1);
    }
    printf("La partie a démarré.\n");
    printf("Lancement du jeu dans 10 secondes...\n");
    for (int i = 10; i >= 0; i--) {
        printf("%d.....", i);
        fflush(stdout);
        sleep(1);
    }
    while(1){
        // attendre un nouveau joueur
    }

    pthread_exit(NULL);
}
void signal_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGUSR1) {
        pid_t pid = info->si_pid;

        if (num_clients < MAX_PLAYERS) {
            client_pids[num_clients++] = pid;

            pthread_t thread;
            CHECK(pthread_create(&thread, NULL, client_thread, &pid), -1, "Erreur lors de la création du thread client");
        } else {
            printf("Nombre maximum de clients atteint. Ignorer le signal SIGUSR1.\n");
        }
    } else if (sig == SIGINT) {
        printf("Arrêt du serveur\n");

        CHECK(sem_close(semaphore), -1, "Erreur lors de la fermeture du sémaphore");

        int shmid = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, 0666);
        CHECK(shmid, -1, "Erreur lors de la récupération de la mémoire partagée");

        CHECK(shmctl(shmid, IPC_RMID, NULL), -1, "Erreur lors de la suppression de la mémoire partagée");
        sem_unlink(SEMAPHORE_NAME);

        exit(EXIT_SUCCESS);
    }
}

int main() {
    srand(time(NULL));
    printf("Serveur lancé\n");
    dictionnaire = createTable();
    taille_dictionnaire = charger_dictionnaire(FILENAME, dictionnaire);
    CHECK(taille_dictionnaire, -1, "Erreur lors du chargement du dictionnaire");
    printf("Taille du dictionnaire : %d\n", taille_dictionnaire);
    taille_dictionnaire_array = charger_dictionnaire_array(FILENAME, dictionnaire_array);
    CHECK(taille_dictionnaire_array, -1, "Erreur lors du chargement du dictionnaire dans un tableau");
    printf("Taille du dictionnaire dans un tableau : %d\n", taille_dictionnaire_array);

    semaphore = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 0); // Création du sémaphore avec une valeur initiale de 0 pour bloquer le client
    if (semaphore == SEM_FAILED) {
        semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 0);
        CHECK(semaphore, SEM_FAILED, "Erreur lors de la création du sémaphore");
    }

    // Installation du gestionnaire de signal
    struct sigaction action;
    action.sa_sigaction = signal_handler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    CHECK(sigaction(SIGUSR1, &action, NULL), -1, "Erreur lors de l'installation du gestionnaire de signal de SIGUSR1");
    CHECK(sigaction(SIGINT, &action, NULL), -1, "Erreur lors de l'installation du gestionnaire de signal de SIGINT");

    // Création de la mémoire partagée pour stocker les PIDs des clients
    int shmid = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    CHECK(shmid, -1, "Erreur lors de la création de la mémoire partagée");

    // Attachement de la mémoire partagée
    shm_ptr = shmat(shmid, NULL, 0);
    CHECK(shm_ptr, (void *) -1, "Erreur lors de l'attachement de la mémoire partagée");

    // Ajout du pid du serveur dans la mémoire partagée
    pid_t server_pid = getpid();
    memcpy(shm_ptr, &server_pid, sizeof(pid_t));

    // Attente des signaux
    while (1) {
        pause();
    }

    // Détachement de la mémoire partagée
    CHECK(shmdt(shm_ptr), -1, "Erreur lors du détachement de la mémoire partagée");
    // Suppression du sémaphore
    CHECK(sem_unlink(SEMAPHORE_NAME), -1, "Erreur lors de la suppression du sémaphore");
    return 0;
}

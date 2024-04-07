#ifndef CONSTANTS_H
#define CONSTANTS_H

#define SHARED_MEMORY_KEY 1234
#define SHM_SIZE sizeof(GameState)
#define FIFO_PATH "/tmp/myfifo"
#define MAX_TUBE_NAME_LENGTH 20
#define SEMAPHORE_NAME "/game_server_semaphore"

typedef struct ClientInfo {
    pid_t pid;  // PID du client
    char tube_name[MAX_TUBE_NAME_LENGTH];  // Nom du tube nommé associé
} ClientInfo;
#endif /* CONSTANTS_H */

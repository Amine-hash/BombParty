#ifndef COMMON_H
#define COMMON_H
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/msg.h>
// Incluez ici tous les fichiers d'en-tête nécessaires
#include "tubes.h"

#include "game_structures.h"
#include "constants.h"
#include "gestion_dictionnaire.h"

#define CHECK(expr, err_val, msg) if ((expr) == (err_val)) { perror(msg); exit(EXIT_FAILURE); }

#endif /* COMMON_H */

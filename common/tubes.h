#ifndef TUBES_H
#define TUBES_H

#include "common.h"


// Définir les constantes ou macros si nécessaire
#define TUBE_PATH "/tmp/my_named_pipe"

// Déclaration des fonctions
int create_named_pipe(const char *path);
int open_named_pipe(const char *path, int flags);
ssize_t write_to_pipe(int fd, const void *buf, size_t count);
ssize_t read_from_pipe(int fd, void *buf, size_t count);
void close_pipe(int fd);

#endif // TUBES_H

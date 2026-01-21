#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

// Standard Error Macro from your labs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

sem_t* create_dynamic_sem(int initial_value) {
    sem_t *sem = malloc(sizeof(sem_t));
    if (!sem) ERR("malloc sem");

    // Second arg '0' means shared between threads (not processes)
    if (sem_init(sem, 0, initial_value) != 0) 
        ERR("sem init");
    
    return sem;
}

void free_dynamic_sem(sem_t *sem) {
    sem_destroy(sem);
    free(sem);
}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

// Standard Error Macro from your labs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

pthread_barrier_t* create_dynamic_barrier(int count) {
    pthread_barrier_t *bar = malloc(sizeof(pthread_barrier_t));
    if (!bar) ERR("malloc barrier");

    if (pthread_barrier_init(bar, NULL, count) != 0) 
        ERR("barrier init");
    
    return bar;
}

void free_dynamic_barrier(pthread_barrier_t *bar) {
    pthread_barrier_destroy(bar);
    free(bar);
}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

// Standard Error Macro from your labs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

pthread_mutex_t* create_dynamic_mutex() {
    pthread_mutex_t *mtx = malloc(sizeof(pthread_mutex_t));
    if (!mtx) ERR("malloc mutex");

    if (pthread_mutex_init(mtx, NULL) != 0) 
        ERR("mutex init");
    
    return mtx;
}

void free_dynamic_mutex(pthread_mutex_t *mtx) {
    pthread_mutex_destroy(mtx);
    free(mtx);
}

//MutexArray

pthread_mutex_t* create_mutex_array(int n) {
    pthread_mutex_t *mtx_array = malloc(n * sizeof(pthread_mutex_t));
    if (!mtx_array) ERR("malloc mutex array");

    for (int i = 0; i < n; i++) {
        if (pthread_mutex_init(&mtx_array[i], NULL) != 0) 
            ERR("mutex array init");
    }
    return mtx_array;
}

void free_mutex_array(pthread_mutex_t *mtx_array, int n) {
    for (int i = 0; i < n; i++) {
        pthread_mutex_destroy(&mtx_array[i]);
    }
    free(mtx_array);
}
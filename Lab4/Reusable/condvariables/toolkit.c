#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

// Standard Error Macro from your labs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct {
    pthread_mutex_t *mtx;
    pthread_cond_t *cond;
} monitor_t;

monitor_t* create_monitor() {
    monitor_t *mon = malloc(sizeof(monitor_t));
    if (!mon) ERR("malloc monitor struct");

    mon->mtx = malloc(sizeof(pthread_mutex_t));
    mon->cond = malloc(sizeof(pthread_cond_t));
    if (!mon->mtx || !mon->cond) ERR("malloc monitor internals");

    if (pthread_mutex_init(mon->mtx, NULL) != 0) ERR("mutex init");
    if (pthread_cond_init(mon->cond, NULL) != 0) ERR("cond init");

    return mon;
}

void free_monitor(monitor_t *mon) {
    pthread_cond_destroy(mon->cond);
    pthread_mutex_destroy(mon->mtx);
    free(mon->cond);
    free(mon->mtx);
    free(mon);
}
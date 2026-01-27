#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

void* wait_for_signal(void* arg) {
    pthread_mutex_lock(&mtx);
    while (ready == 0) { // Always use a WHILE loop, never IF
        pthread_cond_wait(&cond, &mtx); // Releases mtx, sleeps, re-acquires mtx on wake
    }
    printf("Condition met, proceeding!\n");
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void* send_signal(void* arg) {
    pthread_mutex_lock(&mtx);
    ready = 1;
    pthread_cond_signal(&cond); // Use broadcast() to wake ALL threads
    pthread_mutex_unlock(&mtx);
    return NULL;
}
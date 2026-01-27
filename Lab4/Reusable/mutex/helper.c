#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    int *counter;
    pthread_mutex_t *mutex;
} thread_arg_t;

void* worker(void* arg) {
    thread_arg_t *data = (thread_arg_t*)arg;
    
    pthread_mutex_lock(data->mutex);
    (*(data->counter))++; // Protected Critical Section
    printf("Thread %d incremented counter to %d\n", data->id, *(data->counter));
    pthread_mutex_unlock(data->mutex);
    
    return NULL;
}

// Quick Reference:
// pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // Static init
// pthread_create(&tid, NULL, func, &arg);
// pthread_join(tid, NULL);
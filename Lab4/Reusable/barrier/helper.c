#include <pthread.h>
#include <stdio.h>

// Note: Barrier is part of POSIX, might need -D_POSIX_C_SOURCE=200112L
pthread_barrier_t barrier;

void* phase_worker(void* arg) {
    printf("Thread arrived at barrier.\n");
    
    int result = pthread_barrier_wait(&barrier);
    
    // Exactly one thread gets this return value - useful for cleanup/printing
    if (result == PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("All threads arrived! I am the leader thread.\n");
    }
    
    return NULL;
}

// Init: pthread_barrier_init(&barrier, NULL, count);
// Destroy: pthread_barrier_destroy(&barrier);
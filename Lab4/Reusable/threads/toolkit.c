#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

// Standard Error Macro from your labs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Allocates and starts a single thread. Returns the pointer to the thread ID.
pthread_t* create_single_thread(void *(*func)(void *), void *arg) {
    pthread_t *tid = malloc(sizeof(pthread_t));
    if (!tid) ERR("malloc single thread");

    if (pthread_create(tid, NULL, func, arg) != 0) 
        ERR("pthread_create single");
    
    return tid;
}

// Joins and frees a single dynamic thread
void join_single_thread(pthread_t *tid) {
    if (pthread_join(*tid, NULL) != 0) 
        ERR("pthread_join single");
    free(tid);
}


// Allocates an array of N thread IDs and starts them all.
// Note: 'arg' here is passed to ALL threads. If you need unique args, 
// you usually handle that by passing an array of structs as 'arg' and using logic inside the thread.
pthread_t* create_thread_array(int n, void *(*func)(void *), void *arg) {
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    if (!tids) ERR("malloc thread array");

    for (int i = 0; i < n; i++) {
        // Passing 'arg' directly. 
        // CAUTION: If passing &i, ensure synchronization or use a struct array!
        if (pthread_create(&tids[i], NULL, func, arg) != 0) 
            ERR("pthread_create loop");
    }
    return tids;
}

// Joins all N threads and frees the array
void join_thread_array(pthread_t *tids, int n) {
    for (int i = 0; i < n; i++) {
        if (pthread_join(tids[i], NULL) != 0) 
            ERR("pthread_join loop");
    }
    free(tids);
}
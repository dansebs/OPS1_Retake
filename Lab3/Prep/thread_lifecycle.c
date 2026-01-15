#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/* Standard Error Macro */
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* * SCENARIO A: Standard Joinable Threads 
 * Use when you know exactly how many threads you have (N) and they all start at once.
 */
void create_joinable_threads(int n) {
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    if(!tids) ERR("malloc");

    for(int i = 0; i < n; i++) {
        /* Pass NULL or a specific arg struct here */
        if(pthread_create(&tids[i], NULL, (void* (*)(void*))sleep, (void*)1)) 
            ERR("pthread_create");
    }

    /* Wait for ALL to finish */
    for(int i = 0; i < n; i++) {
        pthread_join(tids[i], NULL);
    }
    
    free(tids);
    printf("All joinable threads finished.\n");
}

/* * SCENARIO B: Dynamic Detached Threads (The "Manual Wait")
 * Use when threads are spawned randomly over time (like Task 2/3) 
 * and you can't keep an array of IDs to join.
 */
typedef struct {
    int active_threads;       // The counter
    pthread_mutex_t mx_stats; // The lock for the counter
} thread_manager_t;

void* worker_detached(void* arg) {
    thread_manager_t *mgr = (thread_manager_t*)arg;
    
    /* Do work... */
    usleep(100000); 

    /* DECREMENT ON EXIT */
    pthread_mutex_lock(&mgr->mx_stats);
    mgr->active_threads--;
    pthread_mutex_unlock(&mgr->mx_stats);
    return NULL;
}

void manage_dynamic_threads(int total_to_spawn) {
    thread_manager_t mgr = {0, PTHREAD_MUTEX_INITIALIZER};
    
    for(int i = 0; i < total_to_spawn; i++) {
        pthread_t t;
        
        /* INCREMENT BEFORE SPAWN */
        pthread_mutex_lock(&mgr.mx_stats);
        mgr.active_threads++;
        pthread_mutex_unlock(&mgr.mx_stats);

        /* Create and Detach immediately */
        if(pthread_create(&t, NULL, worker_detached, &mgr)) ERR("create");
        pthread_detach(t);
    }

    /* MANUAL WAIT LOOP (The "Join" replacement) */
    printf("Waiting for detached threads...\n");
    while(1) {
        pthread_mutex_lock(&mgr.mx_stats);
        int left = mgr.active_threads;
        pthread_mutex_unlock(&mgr.mx_stats);

        if(left == 0) break; // All done
        usleep(10000);       // Sleep 10ms to avoid busy waiting
    }
    printf("All detached threads finished.\n");
}
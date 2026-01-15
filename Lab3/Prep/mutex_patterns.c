#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define ERR(source) (perror(source), exit(EXIT_FAILURE))

/* * PATTERN 1: Array of Mutexes (Fine-Grained)
 * Essential for Tasks like the "Dog Race" or "Array Swap".
 */
void setup_mutex_array(int n, pthread_mutex_t **mutex_array) {
    *mutex_array = malloc(n * sizeof(pthread_mutex_t));
    if(!*mutex_array) ERR("malloc mutex");

    for(int i = 0; i < n; i++) {
        /* Initialize each individual mutex in the array */
        if(pthread_mutex_init(&(*mutex_array)[i], NULL)) ERR("mutex init");
    }
}

/* * PATTERN 2: Deadlock Prevention (Lock Order)
 * CRITICAL: Whenever you need two locks, always lock Small ID -> Big ID.
 */
void atomic_swap(int idx_a, int idx_b, pthread_mutex_t *mutexes) {
    int first = (idx_a < idx_b) ? idx_a : idx_b;
    int second = (idx_a < idx_b) ? idx_b : idx_a;

    if (first == second) return; // Don't lock the same mutex twice!

    pthread_mutex_lock(&mutexes[first]);
    pthread_mutex_lock(&mutexes[second]);

    /* ... Perform Swap or Move Operation ... */

    /* Unlock (Order doesn't strictly matter here, but reverse is good habit) */
    pthread_mutex_unlock(&mutexes[second]);
    pthread_mutex_unlock(&mutexes[first]);
}

/* * PATTERN 3: Safe Cleanup
 */
void cleanup_mutex_array(int n, pthread_mutex_t *mutexes) {
    for(int i = 0; i < n; i++) {
        pthread_mutex_destroy(&mutexes[i]);
    }
    free(mutexes);
}
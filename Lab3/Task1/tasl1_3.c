#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h> // Required for sqrt()
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Structure to pass arguments to threads */
typedef struct {
    int k;
    double *tasks;
    double *results;        // Pointer to results array
    pthread_mutex_t *mx;    // Pointer to the synchronization mutex
    unsigned int seed;
} thread_arg_t;

void ReadArguments(int argc, char **argv, int *n, int *k);
void* thread_func(void* arg);

int main(int argc, char** argv) {
    int n, k;
    ReadArguments(argc, argv, &n, &k);

    /* 1. Allocation */
    double *tasks = malloc(k * sizeof(double));
    double *results = malloc(k * sizeof(double)); // New array
    thread_arg_t *args = malloc(n * sizeof(thread_arg_t));
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    
    if (!tasks || !results || !args || !tids) ERR("malloc");

    /* 2. Initialization */
    srand(time(NULL));
    
    // Initialize tasks with random numbers and results with 0
    for(int i = 0; i < k; i++) {
        tasks[i] = 1.0 + ((double)rand() / RAND_MAX) * 59.0;
        results[i] = 0.0;
    }

    // Initialize Mutex
    pthread_mutex_t mx_global = PTHREAD_MUTEX_INITIALIZER;

    /* 3. Create Threads */
    for(int i = 0; i < n; i++) {
        args[i].k = k;
        args[i].tasks = tasks;
        args[i].results = results;  // Pass the results array
        args[i].mx = &mx_global;    // Pass address of the mutex
        args[i].seed = rand();
        
        if(pthread_create(&tids[i], NULL, thread_func, &args[i])) ERR("pthread_create");
    }

    /* 4. Join Threads */
    for(int i = 0; i < n; i++) {
        if(pthread_join(tids[i], NULL)) ERR("pthread_join");
    }

    /* 5. Print Final Arrays */
    printf("\n--- Final Arrays ---\n");
    printf("Tasks:   [ ");
    for(int i=0; i<k; i++) printf("%6.2f ", tasks[i]);
    printf("]\n");
    
    printf("Results: [ ");
    for(int i=0; i<k; i++) printf("%6.2f ", results[i]);
    printf("]\n");

    /* 6. Cleanup */
    // Note: PTHREAD_MUTEX_INITIALIZER doesn't strictly need destroy, but dynamic ones do. 
    // It's good practice generally, though typically used for dynamic mutexes.
    free(tasks);
    free(results);
    free(args);
    free(tids);
    
    return EXIT_SUCCESS;
}

void* thread_func(void* void_arg) {
    thread_arg_t *arg = (thread_arg_t*)void_arg;
    
    int index = rand_r(&arg->seed) % arg->k;
    
    /* --- CRITICAL SECTION START --- */
    pthread_mutex_lock(arg->mx);
    
    double val = arg->tasks[index];
    double res = sqrt(val);
    arg->results[index] = res;
    
    printf("Thread %lu: sqrt(%.2f) = %.2f (Index %d)\n", 
           pthread_self(), val, res, index);
    
    pthread_mutex_unlock(arg->mx);
    /* --- CRITICAL SECTION END --- */

    return NULL;
}

void ReadArguments(int argc, char **argv, int *n, int *k) {
    *n = 3; 
    *k = 7;
    if (argc >= 2) *n = atoi(argv[1]);
    if (argc >= 3) *k = atoi(argv[2]);
    if (*n <= 0 || *k <= 0) {
        fprintf(stderr, "Invalid parameters\n");
        exit(EXIT_FAILURE);
    }
}
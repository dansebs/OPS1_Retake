#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Structure to pass arguments to threads */
typedef struct {
    int k;              // Size of the task array
    double *tasks;      // Pointer to the start of the task array
    unsigned int seed;  // Unique seed for this thread's random number generator
} thread_arg_t;

void ReadArguments(int argc, char **argv, int *n, int *k);
void* thread_func(void* arg);

int main(int argc, char** argv) {
    int n, k;
    ReadArguments(argc, argv, &n, &k);

    /* 1. Allocate the Task Array */
    double *tasks = malloc(k * sizeof(double));
    if (!tasks) ERR("malloc tasks");

    /* 2. Fill Task Array with random numbers [1, 60] */
    srand(time(NULL)); // Seed the global generator for the main thread
    printf("Tasks: [ ");
    for(int i = 0; i < k; i++) {
        /* Formula: 1.0 + (random_0_to_1 * 59.0) */
        tasks[i] = 1.0 + ((double)rand() / RAND_MAX) * 59.0;
        printf("%.2f ", tasks[i]);
    }
    printf("]\n");

    /* 3. Prepare Arguments and Create Threads */
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    thread_arg_t *args = malloc(n * sizeof(thread_arg_t)); // Array of arguments
    if (!tids || !args) ERR("malloc metadata");

    for(int i = 0; i < n; i++) {
        /* Set up the arguments for THIS specific thread */
        args[i].k = k;
        args[i].tasks = tasks; // All threads point to the same array
        args[i].seed = rand(); // Give each thread a unique starting seed
        
        if(pthread_create(&tids[i], NULL, thread_func, &args[i])) 
            ERR("pthread_create");
    }

    /* 4. Wait for threads */
    for(int i = 0; i < n; i++) {
        if(pthread_join(tids[i], NULL)) ERR("pthread_join");
    }

    printf("\nMain: All threads finished.\n");

    /* 5. Cleanup */
    free(tasks);
    free(tids);
    free(args);
    return EXIT_SUCCESS;
}

void* thread_func(void* void_arg) {
    /* Cast the generic pointer back to our struct */
    thread_arg_t *arg = (thread_arg_t*)void_arg;
    
    /* Generate a random index between 0 and k-1 using thread-safe rand_r */
    int random_index = rand_r(&arg->seed) % arg->k;
    
    printf("Index: %d\n", random_index);
    
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
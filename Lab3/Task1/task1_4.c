#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Structure to pass arguments to threads */
typedef struct {
    int k;
    double *tasks;
    double *results;
    bool *is_done;              // Array to track if a cell is finished
    
    pthread_mutex_t *mx_cells;  // Array of mutexes (one per cell)
    
    int *tasks_left;            // Pointer to shared counter
    pthread_mutex_t *mx_main;   // Mutex to protect the shared counter
    
    unsigned int seed;
} thread_arg_t;

/* Helper for cleaner sleep code (POSIX standard) */
void msleep(long milisec) {
    struct timespec req = {0};
    req.tv_sec = milisec / 1000;
    req.tv_nsec = (milisec % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void ReadArguments(int argc, char **argv, int *n, int *k);
void* thread_func(void* arg);

int main(int argc, char** argv) {
    int n, k;
    ReadArguments(argc, argv, &n, &k);

    /* 1. Allocation */
    double *tasks = malloc(k * sizeof(double));
    double *results = malloc(k * sizeof(double));
    bool *is_done = malloc(k * sizeof(bool));
    
    /* Allocate the array of mutexes */
    pthread_mutex_t *mx_cells = malloc(k * sizeof(pthread_mutex_t));
    
    thread_arg_t *args = malloc(n * sizeof(thread_arg_t));
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    
    if (!tasks || !results || !is_done || !mx_cells || !args || !tids) ERR("malloc");

    /* 2. Initialization */
    srand(time(NULL));
    
    // Initialize tasks and mutexes
    for(int i = 0; i < k; i++) {
        tasks[i] = 1.0 + ((double)rand() / RAND_MAX) * 59.0;
        results[i] = 0.0;
        is_done[i] = false;
        
        // Initialize each mutex in the array
        if(pthread_mutex_init(&mx_cells[i], NULL)) ERR("mutex_init");
    }

    /* Initialize Global Counter and its Mutex */
    int tasks_left = k;
    pthread_mutex_t mx_main = PTHREAD_MUTEX_INITIALIZER;

    /* 3. Create Threads */
    for(int i = 0; i < n; i++) {
        args[i].k = k;
        args[i].tasks = tasks;
        args[i].results = results;
        args[i].is_done = is_done;
        
        args[i].mx_cells = mx_cells;  // Pass the array of mutexes
        
        args[i].tasks_left = &tasks_left;
        args[i].mx_main = &mx_main;
        
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
    for(int i=0; i<k; i++) printf("%5.1f ", tasks[i]);
    printf("]\n");
    
    printf("Results: [ ");
    for(int i=0; i<k; i++) printf("%5.1f ", results[i]);
    printf("]\n");

    /* 6. Cleanup */
    for(int i = 0; i < k; i++) {
        pthread_mutex_destroy(&mx_cells[i]);
    }
    free(mx_cells);
    free(tasks);
    free(results);
    free(is_done);
    free(args);
    free(tids);
    
    return EXIT_SUCCESS;
}

void* thread_func(void* void_arg) {
    thread_arg_t *arg = (thread_arg_t*)void_arg;
    
    while(1) {
        /* Check if work is done (Critical Section for Counter) */
        pthread_mutex_lock(arg->mx_main);
        if (*arg->tasks_left <= 0) {
            pthread_mutex_unlock(arg->mx_main);
            break; // Exit the loop and terminate thread
        }
        pthread_mutex_unlock(arg->mx_main);

        /* Pick a random cell */
        int index = rand_r(&arg->seed) % arg->k;

        /* Try to lock THIS specific cell */
        pthread_mutex_lock(&arg->mx_cells[index]);
        
        if (!arg->is_done[index]) {
            /* If cell is not done, do the work */
            double val = arg->tasks[index];
            double res = sqrt(val);
            arg->results[index] = res;
            arg->is_done[index] = true;
            
            printf("Thread %lu: sqrt(%.2f) = %.2f (Index %d)\n", 
                   pthread_self(), val, res, index);
            
            /* Decrement global counter safely */
            pthread_mutex_lock(arg->mx_main);
            (*arg->tasks_left)--;
            pthread_mutex_unlock(arg->mx_main);
            
            /* Unlock the cell */
            pthread_mutex_unlock(&arg->mx_cells[index]);
            
            /* Sleep ONLY after successful work */
            msleep(100);
        } else {
            /* If cell was already done, just unlock and try again immediately */
            pthread_mutex_unlock(&arg->mx_cells[index]);
        }
    }

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
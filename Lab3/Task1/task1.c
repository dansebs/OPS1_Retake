#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Structure to hold arguments (unused in Stage 1, but good practice to set up now) */
typedef struct {
    int id;
} thread_arg_t;

/* Function declarations */
void ReadArguments(int argc, char **argv, int *n, int *k);
void* thread_func(void* arg);

int main(int argc, char** argv) {
    int n, k;
    
    /* 1. Parse Input */
    ReadArguments(argc, argv, &n, &k);
    printf("Main thread: n=%d, k=%d\n", n, k);

    /* 2. Allocate memory for thread IDs */
    /* We need to store IDs to join them later */
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    if (!tids) ERR("malloc");

    /* 3. Create n threads */
    for(int i = 0; i < n; i++) {
        /* In Stage 1, we pass NULL as argument as we just print '*' */
        if(pthread_create(&tids[i], NULL, thread_func, NULL)) 
            ERR("pthread_create");
    }

    /* 4. Wait for threads to finish */
    for(int i = 0; i < n; i++) {
        if(pthread_join(tids[i], NULL)) 
            ERR("pthread_join");
    }
    
    printf("\nMain: All threads joined.\n");

    /* 5. Cleanup */
    free(tids);
    return EXIT_SUCCESS;
}

/* The Worker Thread Function */
void* thread_func(void* arg) {
    printf("*");
    /* stdout is line-buffered; flush ensures '*' appears immediately without \n */
    fflush(stdout); 
    return NULL;
}

/* Helper to read command line arguments */
void ReadArguments(int argc, char **argv, int *n, int *k) {
    *n = 3; // Default as requested
    *k = 7; // Default as requested

    if (argc >= 2) {
        *n = atoi(argv[1]);
        if (*n <= 0) { fprintf(stderr, "Invalid n\n"); exit(EXIT_FAILURE); }
    }
    if (argc >= 3) {
        *k = atoi(argv[2]);
        if (*k <= 0) { fprintf(stderr, "Invalid k\n"); exit(EXIT_FAILURE); }
    }
}
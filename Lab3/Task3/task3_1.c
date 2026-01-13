#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Application Context (No Globals Rule) */
typedef struct {
    int *track;             // The Racetrack (array of dog counts)
    pthread_mutex_t *mx_track; // Mutexes for each cell (prepared for Stage 3)
    int n;                  // Track length
    int m;                  // Number of dogs
} app_context_t;

/* Arguments for each dog thread */
typedef struct {
    app_context_t *ctx;
    int id;                 // Dog number
    unsigned int seed;      // Random seed
} dog_args_t;

void ReadArguments(int argc, char **argv, int *n, int *m);
void* dog_thread(void* void_arg);

int main(int argc, char** argv) {
    app_context_t ctx;
    ReadArguments(argc, argv, &ctx.n, &ctx.m);

    /* 1. Allocation */
    ctx.track = calloc(ctx.n, sizeof(int)); // Init with 0
    ctx.mx_track = malloc(ctx.n * sizeof(pthread_mutex_t));
    if (!ctx.track || !ctx.mx_track) ERR("malloc");

    /* Initialize Mutexes (Even if unused in Stage 1, good practice) */
    for (int i = 0; i < ctx.n; i++) {
        if (pthread_mutex_init(&ctx.mx_track[i], NULL)) ERR("mutex init");
    }

    pthread_t *tids = malloc(ctx.m * sizeof(pthread_t));
    dog_args_t *args = malloc(ctx.m * sizeof(dog_args_t));
    if (!tids || !args) ERR("malloc threads");

    srand(time(NULL));

    /* 2. Create Dog Threads */
    printf("Creating %d dogs for a track of length %d...\n", ctx.m, ctx.n);
    for (int i = 0; i < ctx.m; i++) {
        args[i].ctx = &ctx;
        args[i].id = i + 1; // Dog numbers usually start at 1
        args[i].seed = rand();
        
        if (pthread_create(&tids[i], NULL, dog_thread, &args[i])) 
            ERR("pthread_create");
    }

    /* 3. Wait for all dogs */
    for (int i = 0; i < ctx.m; i++) {
        if (pthread_join(tids[i], NULL)) ERR("pthread_join");
    }

    /* 4. Print Final Track State */
    printf("Final Track: [");
    for (int i = 0; i < ctx.n; i++) {
        printf(" %d", ctx.track[i]);
    }
    printf(" ]\n");

    /* 5. Cleanup */
    for (int i = 0; i < ctx.n; i++) pthread_mutex_destroy(&ctx.mx_track[i]);
    free(ctx.track);
    free(ctx.mx_track);
    free(tids);
    free(args);

    return EXIT_SUCCESS;
}

void* dog_thread(void* void_arg) {
    dog_args_t *args = (dog_args_t*)void_arg;
    app_context_t *ctx = args->ctx;

    /* STAGE 1 LOGIC:
       Pick random cell, increment, print, exit. */
    
    int pos = rand_r(&args->seed) % ctx->n;
    
    /* Note: Unsafe increment for now (Stage 1), ignoring race conditions */
    ctx->track[pos]++;
    
    printf("Dog %d: I chose position %d!\n", args->id, pos);

    return NULL;
}

void ReadArguments(int argc, char **argv, int *n, int *m) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n> <m>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    *n = atoi(argv[1]);
    *m = atoi(argv[2]);
    if (*n <= 20) { fprintf(stderr, "n must be > 20\n"); exit(EXIT_FAILURE); }
    if (*m <= 2)  { fprintf(stderr, "m must be > 2\n"); exit(EXIT_FAILURE); }
}
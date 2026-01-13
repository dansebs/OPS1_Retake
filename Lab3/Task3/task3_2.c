#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Application Context */
typedef struct {
    int *track;
    pthread_mutex_t *mx_track; // Unused in Stage 2
    int n;
    int m;
    int finished_count;        // To track when to stop Main loop
} app_context_t;

/* Dog Arguments */
typedef struct {
    app_context_t *ctx;
    int id;
    unsigned int seed;
} dog_args_t;

/* Random Sleep Helper [200, 1520] ms */
void dog_sleep(unsigned int *seed) {
    int duration = 200 + (rand_r(seed) % 1321);
    struct timespec req = {0};
    req.tv_sec = duration / 1000;
    req.tv_nsec = (duration % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void ReadArguments(int argc, char **argv, int *n, int *m);
void* dog_thread(void* void_arg);

int main(int argc, char** argv) {
    app_context_t ctx;
    ReadArguments(argc, argv, &ctx.n, &ctx.m);
    ctx.finished_count = 0;

    /* 1. Allocation */
    ctx.track = calloc(ctx.n, sizeof(int));
    ctx.mx_track = malloc(ctx.n * sizeof(pthread_mutex_t));
    if (!ctx.track || !ctx.mx_track) ERR("malloc");
    
    /* Mutex init (Unused in Stage 2 but keeping infrastructure) */
    for (int i = 0; i < ctx.n; i++) pthread_mutex_init(&ctx.mx_track[i], NULL);

    pthread_t *tids = malloc(ctx.m * sizeof(pthread_t));
    dog_args_t *args = malloc(ctx.m * sizeof(dog_args_t));
    if (!tids || !args) ERR("malloc threads");

    srand(time(NULL));

    /* 2. Create Dogs */
    printf("Race started! Length: %d, Dogs: %d\n", ctx.n, ctx.m);
    for (int i = 0; i < ctx.m; i++) {
        args[i].ctx = &ctx;
        args[i].id = i + 1;
        args[i].seed = rand();
        if (pthread_create(&tids[i], NULL, dog_thread, &args[i])) ERR("create");
    }

    /* 3. Main Monitor Loop */
    while (1) {
        sleep(1); // Print every 1000ms

        /* Print Track State */
        printf("Track: [");
        for (int i = 0; i < ctx.n; i++) {
            if (ctx.track[i] > 0) printf(" %d", ctx.track[i]); // Print count
            else printf(" ."); // Print dot for empty
        }
        printf(" ] Finished: %d/%d\n", ctx.finished_count, ctx.m);

        /* Exit condition (Unsafe read is acceptable for Stage 2) */
        if (ctx.finished_count >= ctx.m) break;
    }

    /* 4. Join and Cleanup */
    for (int i = 0; i < ctx.m; i++) pthread_join(tids[i], NULL);

    printf("All dogs finished.\n");
    
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
    int pos = 0;

    /* Start at position 0 */
    ctx->track[0]++; 
    printf("Dog %d: Started at 0\n", args->id);

    while (1) {
        /* 1. Wait [200-1520] ms */
        dog_sleep(&args->seed);

        /* 2. Generate Move [1-5] */
        int jump = 1 + (rand_r(&args->seed) % 5);
        int next_pos = pos + jump;

        /* 3. Bounds Check */
        if (next_pos >= ctx->n) {
            next_pos = ctx->n - 1; // Move as far as possible (to the end)
        }

        /* 4. Collision Check (Unsafe Stage 2) */
        /* If someone is there, AND it's not the finish line (finish line holds multiple) */
        /* Actually prompt implies collision applies everywhere. But usually finish line is exception. */
        /* Let's strictly follow: "Checks if there is another dog... stays in place" */
        if (ctx->track[next_pos] > 0) {
            printf("Dog %d: waf waf waf (Collision at %d)\n", args->id, next_pos);
            continue; // Stay in place, loop again
        }

        /* 5. Update Position */
        ctx->track[pos]--;
        ctx->track[next_pos]++;
        
        printf("Dog %d: Moved %d -> %d\n", args->id, pos, next_pos);
        pos = next_pos;

        /* 6. Check Finish */
        if (pos == ctx->n - 1) {
            ctx->finished_count++; // Increment finished counter
            printf("Dog %d: Finished! (Rank: %d)\n", args->id, ctx->finished_count);
            break; // Exit loop
        }
    }

    return NULL;
}

void ReadArguments(int argc, char **argv, int *n, int *m) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n> <m>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    *n = atoi(argv[1]);
    *m = atoi(argv[2]);
    if (*n <= 20) exit(EXIT_FAILURE);
    if (*m <= 2)  exit(EXIT_FAILURE);
}
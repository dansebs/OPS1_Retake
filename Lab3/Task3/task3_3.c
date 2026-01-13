#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Application Context */
typedef struct {
    int *track;
    pthread_mutex_t *mx_track; // Array of mutexes for the track
    int n;
    int m;
    
    int finished_count;
    pthread_mutex_t mx_finish; // Mutex for the finish counter
} app_context_t;

typedef struct {
    app_context_t *ctx;
    int id;
    unsigned int seed;
} dog_args_t;

/* Sleep helper */
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

    /* 2. Initialization */
    for (int i = 0; i < ctx.n; i++) {
        if (pthread_mutex_init(&ctx.mx_track[i], NULL)) ERR("mutex init");
    }
    if (pthread_mutex_init(&ctx.mx_finish, NULL)) ERR("mutex init finish");

    pthread_t *tids = malloc(ctx.m * sizeof(pthread_t));
    dog_args_t *args = malloc(ctx.m * sizeof(dog_args_t));
    if (!tids || !args) ERR("malloc threads");

    srand(time(NULL));

    /* 3. Create Dogs */
    printf("Race started! Length: %d, Dogs: %d\n", ctx.n, ctx.m);
    for (int i = 0; i < ctx.m; i++) {
        args[i].ctx = &ctx;
        args[i].id = i + 1;
        args[i].seed = rand();
        if (pthread_create(&tids[i], NULL, dog_thread, &args[i])) ERR("create");
    }

    /* 4. Monitor Loop */
    while (1) {
        sleep(1);

        /* LOCK EVERYTHING for snapshot */
        for (int i = 0; i < ctx.n; i++) pthread_mutex_lock(&ctx.mx_track[i]);
        pthread_mutex_lock(&ctx.mx_finish);

        /* Print */
        printf("\nTrack: [");
        for (int i = 0; i < ctx.n; i++) {
            if (ctx.track[i] > 0) printf("%d", ctx.track[i]); // Compact print
            else printf(".");
        }
        printf("] Finished: %d/%d\n", ctx.finished_count, ctx.m);
        
        int f = ctx.finished_count; // Read before unlocking

        /* UNLOCK EVERYTHING */
        pthread_mutex_unlock(&ctx.mx_finish);
        for (int i = 0; i < ctx.n; i++) pthread_mutex_unlock(&ctx.mx_track[i]);

        if (f >= ctx.m) break;
    }

    /* 5. Cleanup */
    for (int i = 0; i < ctx.m; i++) pthread_join(tids[i], NULL);
    
    printf("\nAll dogs finished!\n");

    for (int i = 0; i < ctx.n; i++) pthread_mutex_destroy(&ctx.mx_track[i]);
    pthread_mutex_destroy(&ctx.mx_finish);
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

    /* Start at 0 safely */
    pthread_mutex_lock(&ctx->mx_track[0]);
    ctx->track[0]++;
    pthread_mutex_unlock(&ctx->mx_track[0]);
    
    // printf("Dog %d: Ready at start.\n", args->id);

    while (1) {
        dog_sleep(&args->seed);

        int jump = 1 + (rand_r(&args->seed) % 5);
        int next_pos = pos + jump;

        /* Bounds / Finish Logic */
        if (next_pos >= ctx->n) {
            next_pos = ctx->n - 1;
        }
        
        /* If we are already at the end (shouldn't happen with break logic, but safety check) */
        if (pos == next_pos) break;

        /* --- ATOMIC MOVE START --- */
        
        /* 1. Determine Lock Order (Deadlock Prevention) */
        int first = (pos < next_pos) ? pos : next_pos;
        int second = (pos < next_pos) ? next_pos : pos;

        /* 2. Lock Both */
        pthread_mutex_lock(&ctx->mx_track[first]);
        pthread_mutex_lock(&ctx->mx_track[second]);

        /* 3. Check Collision */
        /* If next_pos is occupied, and it is NOT the finish line */
        /* (Assuming finish line can hold multiple dogs, otherwise race never ends) */
        /* Instructions: "Checks if there is another dog... stays in place" */
        /* Usually finish line (n-1) is an exception in race conditions, 
           but prompt says "Checks if... In such situation... stays in place".
           Strictly following this means dogs block the finish line too! 
           However, standard interpretation is finish line is infinite capacity.
           Let's assume Finish Line (n-1) is exempt from collision. */
        
        if (next_pos != ctx->n - 1 && ctx->track[next_pos] > 0) {
            printf("Dog %d: waf waf waf (Blocked at %d)\n", args->id, next_pos);
            /* Unlock and try again later */
            pthread_mutex_unlock(&ctx->mx_track[second]);
            pthread_mutex_unlock(&ctx->mx_track[first]);
            continue; 
        }

        /* 4. Perform Move */
        ctx->track[pos]--;
        ctx->track[next_pos]++;
        // printf("Dog %d: %d -> %d\n", args->id, pos, next_pos);

        /* 5. Unlock */
        pthread_mutex_unlock(&ctx->mx_track[second]);
        pthread_mutex_unlock(&ctx->mx_track[first]);
        /* --- ATOMIC MOVE END --- */

        pos = next_pos;

        /* 6. Check Finish */
        if (pos == ctx->n - 1) {
            pthread_mutex_lock(&ctx->mx_finish);
            ctx->finished_count++;
            int rank = ctx->finished_count;
            /* If rank <= 3, keep it for final print? Prompt says "print the three fastest" 
               at the end. We'll verify this in Stage 4. For now just print. */
            pthread_mutex_unlock(&ctx->mx_finish);
            
            printf("Dog %d finished! Rank: %d\n", args->id, rank);
            break;
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
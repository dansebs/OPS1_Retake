#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* --- Global Signals --- */
volatile sig_atomic_t sigint_received = 0;

void sig_handler(int sig) {
    if (sig == SIGINT) sigint_received = 1;
}

/* --- Application Context --- */
typedef struct {
    int *track;                 // The race track array
    pthread_mutex_t *mx_track;  // Mutex per cell
    int n;                      // Track length
    int m;                      // Number of dogs
    
    int finished_count;         // How many dogs finished
    int winners[3];             // IDs of the top 3 dogs
    pthread_mutex_t mx_finish;  // Mutex to protect finish logic
} app_context_t;

/* --- Arguments for Dog Threads --- */
typedef struct {
    app_context_t *ctx;
    int id;
    unsigned int seed;
} dog_args_t;

/* --- Helper Functions --- */
void dog_sleep(unsigned int *seed) {
    int duration = 200 + (rand_r(seed) % 1321); // [200, 1520] ms
    struct timespec req = {0};
    req.tv_sec = duration / 1000;
    req.tv_nsec = (duration % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void ReadArguments(int argc, char **argv, int *n, int *m) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n> <m>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    *n = atoi(argv[1]);
    *m = atoi(argv[2]);
    if (*n <= 20 || *m <= 2) {
        fprintf(stderr, "Invalid args: n must be > 20, m must be > 2\n");
        exit(EXIT_FAILURE);
    }
}

/* --- Dog Thread Logic --- */
void* dog_thread(void* void_arg) {
    dog_args_t *args = (dog_args_t*)void_arg;
    app_context_t *ctx = args->ctx;
    int pos = 0;

    /* Start at position 0 */
    pthread_mutex_lock(&ctx->mx_track[0]);
    ctx->track[0]++;
    pthread_mutex_unlock(&ctx->mx_track[0]);

    while (1) {
        /* 1. Sleep (Cancellation Point) */
        dog_sleep(&args->seed);

        /* 2. Calculate potential move */
        int jump = 1 + (rand_r(&args->seed) % 5);
        int next_pos = pos + jump;
        
        /* Bounds Check */
        if (next_pos >= ctx->n) next_pos = ctx->n - 1;
        
        /* If we are already at the end, stop (sanity check) */
        if (pos == next_pos) break;

        /* --- CRITICAL SECTION START --- */
        /* Disable cancellation so we don't die holding a lock */
        int old_state;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);

        /* Determine Locking Order (Deadlock Prevention) */
        int first = (pos < next_pos) ? pos : next_pos;
        int second = (pos < next_pos) ? next_pos : pos;

        pthread_mutex_lock(&ctx->mx_track[first]);
        pthread_mutex_lock(&ctx->mx_track[second]);

        /* Collision Check */
        /* Only collide if next_pos is occupied AND it is NOT the finish line */
        if (next_pos != ctx->n - 1 && ctx->track[next_pos] > 0) {
            printf("Dog %d: waf waf waf\n", args->id);
            
            /* Unlock and retry later */
            pthread_mutex_unlock(&ctx->mx_track[second]);
            pthread_mutex_unlock(&ctx->mx_track[first]);
            
            pthread_setcancelstate(old_state, NULL); // Re-enable cancel
            continue;
        }

        /* Move the dog */
        ctx->track[pos]--;
        ctx->track[next_pos]++;

        pthread_mutex_unlock(&ctx->mx_track[second]);
        pthread_mutex_unlock(&ctx->mx_track[first]);

        /* Re-enable cancellation */
        pthread_setcancelstate(old_state, NULL);
        /* --- CRITICAL SECTION END --- */

        pos = next_pos;

        /* Check Finish */
        if (pos == ctx->n - 1) {
            pthread_mutex_lock(&ctx->mx_finish);
            ctx->finished_count++;
            
            /* Record Top 3 */
            if (ctx->finished_count <= 3) {
                ctx->winners[ctx->finished_count - 1] = args->id;
            }
            pthread_mutex_unlock(&ctx->mx_finish);
            
            /* Dog finishes work here */
            break;
        }
    }
    return NULL;
}

/* --- Main Function --- */
int main(int argc, char** argv) {
    app_context_t ctx;
    ReadArguments(argc, argv, &ctx.n, &ctx.m);
    
    ctx.finished_count = 0;
    for(int i=0; i<3; i++) ctx.winners[i] = 0;

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

    /* 3. Signal Setup */
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) ERR("sigaction");

    srand(time(NULL));

    /* 4. Start Race */
    printf("Race started! Length: %d, Dogs: %d\n", ctx.n, ctx.m);
    for (int i = 0; i < ctx.m; i++) {
        args[i].ctx = &ctx;
        args[i].id = i + 1;
        args[i].seed = rand();
        if (pthread_create(&tids[i], NULL, dog_thread, &args[i])) ERR("create");
    }

    /* 5. Monitor Loop */
    while (1) {
        /* Responsive Sleep (check SIGINT every 0.1s) */
        for(int k=0; k<10; k++) {
            if (sigint_received) break;
            usleep(100000); 
        }
        
        /* Handle Cancellation */
        if (sigint_received) {
            printf("\n\n=== RACE INTERRUPTED (SIGINT) ===\n");
            printf("Cancelling all dogs...\n");
            for (int i = 0; i < ctx.m; i++) {
                pthread_cancel(tids[i]);
            }
            break;
        }

        /* Lock All to Print */
        for (int i = 0; i < ctx.n; i++) pthread_mutex_lock(&ctx.mx_track[i]);
        pthread_mutex_lock(&ctx.mx_finish);

        /* Print Track (Standard scrolling with \n) */
        printf("Track: [");
        for (int i = 0; i < ctx.n; i++) {
            if (ctx.track[i] > 0) printf("%d", ctx.track[i]);
            else printf(".");
        }
        printf("] Finished: %d/%d\n", ctx.finished_count, ctx.m);
        
        int f = ctx.finished_count;

        pthread_mutex_unlock(&ctx.mx_finish);
        for (int i = 0; i < ctx.n; i++) pthread_mutex_unlock(&ctx.mx_track[i]);

        /* Stop if everyone finished */
        if (f >= ctx.m) break;
    }

    /* 6. Wait for all threads (Finish or Cancel) */
    for (int i = 0; i < ctx.m; i++) {
        pthread_join(tids[i], NULL);
    }

    /* 7. Final Results */
    if (!sigint_received) {
        printf("\n\n=== RACE FINISHED ===\n");
        printf("1st Place: Dog %d\n", ctx.winners[0]);
        printf("2nd Place: Dog %d\n", ctx.winners[1]);
        printf("3rd Place: Dog %d\n", ctx.winners[2]);
    } else {
        printf("Race cancelled. Cleanup complete.\n");
    }

    /* 8. Cleanup */
    for (int i = 0; i < ctx.n; i++) pthread_mutex_destroy(&ctx.mx_track[i]);
    pthread_mutex_destroy(&ctx.mx_finish);
    
    free(ctx.track);
    free(ctx.mx_track);
    free(tids);
    free(args);

    return EXIT_SUCCESS;
}
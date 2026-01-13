#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* --- Global Signals --- */
volatile sig_atomic_t sig1_received = 0;
volatile sig_atomic_t sig2_received = 0;

/* --- Context Structure --- */
typedef struct {
    int *array;
    pthread_mutex_t *mx_array;
    int N;
    int P; // Max thread limit
    
    int active_threads;         // Current count of swapper threads
    pthread_mutex_t mx_stats;   // Mutex to protect the counter
} app_context_t;

/* --- Arguments for Swapper Thread --- */
typedef struct {
    app_context_t *ctx;
    int a;
    int b;
} swap_args_t;

void msleep(long milisec) {
    struct timespec req = {0};
    req.tv_sec = milisec / 1000;
    req.tv_nsec = (milisec % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void ReadArguments(int argc, char **argv, int *n, int *p) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n> <p>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    *n = atoi(argv[1]);
    *p = atoi(argv[2]);
    if (*n < 8 || *n > 256) exit(EXIT_FAILURE);
    if (*p < 1 || *p > 16) exit(EXIT_FAILURE);
}

void sig_handler(int sig) {
    if (sig == SIGUSR1) sig1_received = 1;
    if (sig == SIGUSR2) sig2_received = 1;
}

/* --- Thread Functions --- */

/* The work logic (Refactored to be called by thread) */
void perform_inversion(app_context_t *ctx, int a, int b) {
    int left = a;
    int right = b;
    // printf("Thread %lu: Swapping [%d, %d]\n", pthread_self(), a, b);

    while (left < right) {
        pthread_mutex_lock(&ctx->mx_array[left]);
        pthread_mutex_lock(&ctx->mx_array[right]);

        int temp = ctx->array[left];
        ctx->array[left] = ctx->array[right];
        ctx->array[right] = temp;

        pthread_mutex_unlock(&ctx->mx_array[right]);
        pthread_mutex_unlock(&ctx->mx_array[left]);

        left++;
        right--;
        msleep(5);
    }
}

/* The Swapper Thread Entry Point */
void* swapper_thread(void* void_arg) {
    swap_args_t *args = (swap_args_t*)void_arg;
    app_context_t *ctx = args->ctx;

    /* 1. Do the work */
    perform_inversion(ctx, args->a, args->b);

    /* 2. Update Stats (Decrement active threads) */
    pthread_mutex_lock(&ctx->mx_stats);
    ctx->active_threads--;
    printf("Thread finished. Active threads: %d/%d\n", ctx->active_threads, ctx->P);
    pthread_mutex_unlock(&ctx->mx_stats);

    /* 3. Cleanup */
    free(args); // Free the struct allocated in main
    return NULL;
}

void* printer_thread(void* void_arg) {
    app_context_t *ctx = (app_context_t*)void_arg;
    
    /* Lock Entire Array */
    for(int i = 0; i < ctx->N; i++) pthread_mutex_lock(&ctx->mx_array[i]);

    printf("[Array State]:");
    for(int i = 0; i < ctx->N; i++) printf(" %d", ctx->array[i]);
    printf("\n");

    /* Unlock Entire Array */
    for(int i = 0; i < ctx->N; i++) pthread_mutex_unlock(&ctx->mx_array[i]);

    return NULL;
}

/* --- Main --- */

int main(int argc, char** argv) {
    app_context_t ctx;
    ReadArguments(argc, argv, &ctx.N, &ctx.P);

    /* Initialization */
    ctx.array = malloc(ctx.N * sizeof(int));
    ctx.mx_array = malloc(ctx.N * sizeof(pthread_mutex_t));
    if (!ctx.array || !ctx.mx_array) ERR("malloc");

    for (int i = 0; i < ctx.N; i++) {
        ctx.array[i] = i;
        if (pthread_mutex_init(&ctx.mx_array[i], NULL)) ERR("mutex init");
    }

    /* Initialize Stats */
    ctx.active_threads = 0;
    if (pthread_mutex_init(&ctx.mx_stats, NULL)) ERR("mutex init stats");

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) ERR("sigaction");
    if (sigaction(SIGUSR2, &sa, NULL) == -1) ERR("sigaction");

    srand(time(NULL));
    printf("PID: %d. Ready. Max threads: %d\n", getpid(), ctx.P);

    while (1) {
        pause();

        if (sig1_received) {
            sig1_received = 0;
            
            /* 1. Check if we have capacity */
            pthread_mutex_lock(&ctx.mx_stats);
            if (ctx.active_threads >= ctx.P) {
                printf("All threads busy, aborting request.\n");
                pthread_mutex_unlock(&ctx.mx_stats);
                continue; // Skip this request
            }
            
            /* Reserve the slot immediately */
            ctx.active_threads++;
            printf("Spawning thread. Active: %d/%d\n", ctx.active_threads, ctx.P);
            pthread_mutex_unlock(&ctx.mx_stats);

            /* 2. Prepare Arguments */
            int a = rand() % ctx.N;
            int b = rand() % ctx.N;
            if (a == b) a = (a + 1) % ctx.N; // Ensure a != b simply
            if (a > b) { int t = a; a = b; b = t; }

            swap_args_t *args = malloc(sizeof(swap_args_t));
            if (!args) ERR("malloc args");
            args->ctx = &ctx;
            args->a = a;
            args->b = b;

            /* 3. Spawn Thread */
            pthread_t t;
            if (pthread_create(&t, NULL, swapper_thread, args)) {
                /* If creation fails, we must rollback the counter! */
                pthread_mutex_lock(&ctx.mx_stats);
                ctx.active_threads--;
                pthread_mutex_unlock(&ctx.mx_stats);
                free(args);
                ERR("pthread_create swapper");
            }
            pthread_detach(t);
        }

        if (sig2_received) {
            sig2_received = 0;
            pthread_t t;
            if (pthread_create(&t, NULL, printer_thread, &ctx)) ERR("create printer");
            pthread_detach(t);
        }
    }

    /* Cleanup code (unreachable in infinite loop) */
    return EXIT_SUCCESS;
}
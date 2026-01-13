#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Signals */
volatile sig_atomic_t sig1_received = 0;
volatile sig_atomic_t sig2_received = 0;
volatile sig_atomic_t sigint_received = 0; // New flag for Ctrl+C

/* Context Structure */
typedef struct {
    int *array;
    pthread_mutex_t *mx_array;
    int N;
    int P;
    
    int active_threads;
    pthread_mutex_t mx_stats;
} app_context_t;

/* Arguments for Swapper Thread */
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
    if (sig == SIGINT)  sigint_received = 1; // Handle Ctrl+C
}

/* --- Thread Logic --- */

void perform_inversion(app_context_t *ctx, int a, int b) {
    int left = a;
    int right = b;
    while (left < right) {
        /* DEADLOCK PREVENTION: Lock smaller index first */
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

void* swapper_thread(void* void_arg) {
    swap_args_t *args = (swap_args_t*)void_arg;
    app_context_t *ctx = args->ctx;

    perform_inversion(ctx, args->a, args->b);

    /* Update Stats (Thread Finished) */
    pthread_mutex_lock(&ctx->mx_stats);
    ctx->active_threads--;
    printf("Thread finished. Active threads: %d/%d\n", ctx->active_threads, ctx->P);
    pthread_mutex_unlock(&ctx->mx_stats);

    free(args);
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

    ctx.active_threads = 0;
    if (pthread_mutex_init(&ctx.mx_stats, NULL)) ERR("mutex init stats");

    /* Signal Setup */
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) ERR("sigaction");
    if (sigaction(SIGUSR2, &sa, NULL) == -1) ERR("sigaction");
    if (sigaction(SIGINT, &sa, NULL) == -1)  ERR("sigaction"); // Register SIGINT

    srand(time(NULL));
    printf("PID: %d. Ready. (N=%d, P=%d)\n", getpid(), ctx.N, ctx.P);

    while (1) {
        pause();

        /* --- STAGE 4: TERMINATION CHECK --- */
        if (sigint_received) {
            printf("\nSIGINT received. Waiting for active threads to finish...\n");
            
            /* Wait loop */
            while(1) {
                pthread_mutex_lock(&ctx.mx_stats);
                int left = ctx.active_threads;
                pthread_mutex_unlock(&ctx.mx_stats);
                
                if (left == 0) break; // All done
                
                msleep(100); // Check again in 100ms
            }
            
            printf("All threads finished. Cleaning up.\n");
            break; // Break the infinite loop
        }

        /* Swapper Request */
        if (sig1_received) {
            sig1_received = 0;
            
            pthread_mutex_lock(&ctx.mx_stats);
            if (ctx.active_threads >= ctx.P) {
                printf("All threads busy, aborting request.\n");
                pthread_mutex_unlock(&ctx.mx_stats);
                continue;
            }
            ctx.active_threads++;
            // printf("Spawning thread. Active: %d/%d\n", ctx.active_threads, ctx.P);
            pthread_mutex_unlock(&ctx.mx_stats);

            int a = rand() % ctx.N;
            int b = rand() % ctx.N;
            if (a == b) a = (a + 1) % ctx.N;
            if (a > b) { int t = a; a = b; b = t; }

            swap_args_t *args = malloc(sizeof(swap_args_t));
            if (!args) ERR("malloc args");
            args->ctx = &ctx;
            args->a = a;
            args->b = b;

            pthread_t t;
            if (pthread_create(&t, NULL, swapper_thread, args)) {
                pthread_mutex_lock(&ctx.mx_stats);
                ctx.active_threads--;
                pthread_mutex_unlock(&ctx.mx_stats);
                free(args);
                ERR("pthread_create swapper");
            }
            pthread_detach(t);
        }

        /* Printer Request */
        if (sig2_received) {
            sig2_received = 0;
            pthread_t t;
            if (pthread_create(&t, NULL, printer_thread, &ctx)) ERR("create printer");
            pthread_detach(t);
        }
    }

    /* Clean Exit */
    for (int i = 0; i < ctx.N; i++) {
        pthread_mutex_destroy(&ctx.mx_array[i]);
    }
    pthread_mutex_destroy(&ctx.mx_stats);
    
    free(ctx.array);
    free(ctx.mx_array);
    
    printf("Program terminated cleanly.\n");
    return EXIT_SUCCESS;
}
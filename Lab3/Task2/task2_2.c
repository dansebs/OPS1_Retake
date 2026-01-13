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
/* Strictly allowed only for signal communication */
volatile sig_atomic_t sig1_received = 0;
volatile sig_atomic_t sig2_received = 0;

/* --- Context Structure --- */
/* Holds the state of the application to avoid other globals */
typedef struct {
    int *array;
    pthread_mutex_t *mx_array;
    int N; // Array size
    int P; // Max threads (unused in Stage 2, but prepared)
} app_context_t;

/* --- Helper Functions --- */

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
    
    if (*n < 8 || *n > 256) { fprintf(stderr, "Invalid n (8-256)\n"); exit(EXIT_FAILURE); }
    if (*p < 1 || *p > 16)  { fprintf(stderr, "Invalid p (1-16)\n"); exit(EXIT_FAILURE); }
}

/* Signal Handler */
void sig_handler(int sig) {
    if (sig == SIGUSR1) sig1_received = 1;
    if (sig == SIGUSR2) sig2_received = 1;
}

/* --- Logic Functions --- */

/* Performed by Main Thread in Stage 2 */
void perform_inversion(app_context_t *ctx, int a, int b) {
    printf("PID %d: Inverting range [%d, %d]\n", getpid(), a, b);
    
    int left = a;
    int right = b;

    while (left < right) {
        /* DEADLOCK PREVENTION: Always lock lower index first */
        pthread_mutex_lock(&ctx->mx_array[left]);
        pthread_mutex_lock(&ctx->mx_array[right]);

        /* Swap */
        int temp = ctx->array[left];
        ctx->array[left] = ctx->array[right];
        ctx->array[right] = temp;

        /* Unlock (Order doesn't strictly matter for unlock, but reverse is good habit) */
        pthread_mutex_unlock(&ctx->mx_array[right]);
        pthread_mutex_unlock(&ctx->mx_array[left]);

        left++;
        right--;
        
        msleep(5); // Simulate work
    }
    printf("PID %d: Inversion [%d, %d] complete.\n", getpid(), a, b);
}

/* Performed by Detached Thread in Stage 2 */
void* printer_thread(void* void_arg) {
    app_context_t *ctx = (app_context_t*)void_arg;
    
    /* 1. LOCK THE WHOLE ARRAY */
    /* We must lock every single mutex to ensure we see a consistent state.
       We lock from 0 to N-1 to avoid deadlocks with the swapper. */
    for(int i = 0; i < ctx->N; i++) {
        pthread_mutex_lock(&ctx->mx_array[i]);
    }

    /* 2. PRINT */
    printf("[Array State]:");
    for(int i = 0; i < ctx->N; i++) {
        printf(" %d", ctx->array[i]);
    }
    printf("\n");

    /* 3. UNLOCK THE WHOLE ARRAY */
    for(int i = 0; i < ctx->N; i++) {
        pthread_mutex_unlock(&ctx->mx_array[i]);
    }

    return NULL;
}

/* --- Main --- */

int main(int argc, char** argv) {
    app_context_t ctx;
    ReadArguments(argc, argv, &ctx.N, &ctx.P);

    /* 1. Allocation & Initialization */
    ctx.array = malloc(ctx.N * sizeof(int));
    ctx.mx_array = malloc(ctx.N * sizeof(pthread_mutex_t));
    if (!ctx.array || !ctx.mx_array) ERR("malloc");

    for (int i = 0; i < ctx.N; i++) {
        ctx.array[i] = i;
        if (pthread_mutex_init(&ctx.mx_array[i], NULL)) ERR("mutex init");
    }

    /* 2. Signal Setup */
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) ERR("sigaction USR1");
    if (sigaction(SIGUSR2, &sa, NULL) == -1) ERR("sigaction USR2");

    srand(time(NULL));
    printf("PID: %d. Ready. (N=%d, P=%d)\n", getpid(), ctx.N, ctx.P);

    /* 3. Event Loop */
    while (1) {
        /* Suspend execution until a signal arrives */
        pause();

        /* Check SIGUSR1 (Inversion) */
        if (sig1_received) {
            sig1_received = 0;
            int a = rand() % ctx.N;
            int b = rand() % ctx.N;
            
            if (a == b) continue;
            if (a > b) { int t = a; a = b; b = t; }

            // Stage 2: Main thread does the work
            perform_inversion(&ctx, a, b);
        }

        /* Check SIGUSR2 (Print) */
        if (sig2_received) {
            sig2_received = 0;
            pthread_t t;
            
            // Spawn a new thread passing the context
            if (pthread_create(&t, NULL, printer_thread, &ctx)) 
                ERR("pthread_create printer");
            
            // Detach it so we don't have to join it
            if (pthread_detach(t)) 
                ERR("pthread_detach");
        }
    }

    /* Cleanup (unreachable infinite loop) */
    free(ctx.array);
    free(ctx.mx_array);
    return EXIT_SUCCESS;
}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* Only strictly necessary global variable */
volatile sig_atomic_t sig1_received = 0;

/* Context Structure: Holds all "Global" application state */
typedef struct {
    int *array;
    pthread_mutex_t *mx_array;
    int N;
    int P;
} app_context_t;

void msleep(long milisec) {
    struct timespec req = {0};
    req.tv_sec = milisec / 1000;
    req.tv_nsec = (milisec % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void sig_handler(int sig) {
    if (sig == SIGUSR1) {
        sig1_received = 1;
    }
}

/* Now accepts the context instead of using globals */
void perform_inversion(app_context_t *ctx, int a, int b) {
    printf("Inverting range [%d, %d]\n", a, b);
    
    int left = a;
    int right = b;

    while (left < right) {
        /* Lock lower index first to avoid deadlock */
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
    printf("Inversion [%d, %d] complete.\n", a, b);
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

int main(int argc, char** argv) {
    /* 1. Create Context on Stack (or Heap) in Main */
    app_context_t ctx;
    ReadArguments(argc, argv, &ctx.N, &ctx.P);

    /* 2. Initialization */
    ctx.array = malloc(ctx.N * sizeof(int));
    ctx.mx_array = malloc(ctx.N * sizeof(pthread_mutex_t));
    if (!ctx.array || !ctx.mx_array) ERR("malloc");

    for (int i = 0; i < ctx.N; i++) {
        ctx.array[i] = i;
        if (pthread_mutex_init(&ctx.mx_array[i], NULL)) ERR("mutex init");
    }

    /* 3. Signal Setup */
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) ERR("sigaction");

    printf("PID: %d. Waiting...\n", getpid());
    srand(time(NULL));

    /* 4. Main Loop */
    while (1) {
        pause(); // Wait for signal

        if (sig1_received) {
            sig1_received = 0;
            int a = rand() % ctx.N;
            int b = rand() % ctx.N;
            if (a == b) continue;
            if (a > b) { int t = a; a = b; b = t; }

            /* Pass the context pointer explicitly */
            perform_inversion(&ctx, a, b);
        }
    }

    free(ctx.array);
    free(ctx.mx_array);
    return EXIT_SUCCESS;
}
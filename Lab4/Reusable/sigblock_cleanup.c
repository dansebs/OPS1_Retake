// robust_main_template.c
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Global shutdown flag for all threads to check
volatile sig_atomic_t do_work = 1;

void setup_signal_mask(sigset_t *mask) {
    sigemptyset(mask);
    sigaddset(mask, SIGINT);  // Ctrl+C
    sigaddset(mask, SIGUSR1); // Custom Event
    if (pthread_sigmask(SIG_BLOCK, mask, NULL) != 0) ERR("sigmask");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s <n_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);

    // 1. Block Signals (Critical for Producer-Consumer)
    sigset_t mask;
    setup_signal_mask(&mask);

    // 2. Initialize Resources (Pool, Mutexes, Semaphores)
    // [Insert Init Logic Here]

    // 3. The Event Loop (The "Producer" Logic)
    while (do_work) {
        int sig;
        if (sigwait(&mask, &sig) != 0) ERR("sigwait");

        switch (sig) {
            case SIGINT:
                printf("\n[Producer] Caught SIGINT. Shutting down...\n");
                do_work = 0;
                break;
            case SIGUSR1:
                printf("[Producer] Received Task Signal.\n");
                // [Insert Task Submission Logic Here]
                break;
            default:
                break;
        }
    }

    // 4. Cleanup & Join
    // [Insert Join Loop Here]
    
    return EXIT_SUCCESS;
}
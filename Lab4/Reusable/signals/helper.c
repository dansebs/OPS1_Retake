#include <signal.h>
#include <stdio.h>
#include <pthread.h>

void* signal_handler_thread(void* arg) {
    sigset_t *set = (sigset_t*)arg;
    int sig;
    
    while (1) {
        sigwait(set, &sig); // Synchronously wait for blocked signals
        if (sig == SIGINT) {
            printf("\nCaught SIGINT (Ctrl+C). Cleaning up...\n");
            break; 
        }
    }
    return NULL;
}

// In main():
// sigset_t set;
// sigemptyset(&set);
// sigaddset(&set, SIGINT);
// pthread_sigmask(SIG_BLOCK, &set, NULL); // BLOCK in all threads
// pthread_create(&tid, NULL, signal_handler_thread, &set);
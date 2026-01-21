#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>









// Standard Error Macro from your labs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void sigM(){
// 1. Setup Signal Mask (BLOCK signals BEFORE creating threads)
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1); // Custom event (e.g., "Add Task")
    sigaddset(&mask, SIGINT);  // Shutdown event (Ctrl+C)
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) ERR("sigmask");

    // 2. Create Threads (Workers inherit the mask!)
    // [Insert create_threads logic here]

    // 3. The Event Loop
    int working = 1;
    while(working) {
        int sig;
        if (sigwait(&mask, &sig) != 0) ERR("sigwait");
        
        switch(sig) {
            case SIGUSR1:
                // Handle "New Work" or "Seat Player"
                break;
            case SIGINT:
                // Handle Shutdown
                working = 0; // Break loop
                // [Set global shutdown flag = 1]
                // [Broadcast condition variables to wake sleeping workers]
                break;
        }
    }
// 4. Join threads
}
#define _GNU_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

// Global flag for signal waiting
volatile sig_atomic_t sig_received = 0;

void simple_handler(int sig) {
    sig_received = 1;
}

void main_logic(int n) {
    // 1. SETUP MASKS (Item 8)
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1); 
    sigaddset(&mask, SIGUSR2);
    // Block signals BEFORE fork so children inherit the mask
    sigprocmask(SIG_BLOCK, &mask, &oldmask); 

    // 2. CREATE CHILDREN (Item 6 & 10)
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork"); exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // CHILD PROCESS
            // Unblock signals if child needs to receive them immediately
            // OR keep blocked and use sigsuspend.
            
            // Example: Restore default mask
            sigprocmask(SIG_SETMASK, &oldmask, NULL); 
            
            // Child logic here...
            printf("Child %d (PID: %d) doing work\n", i, getpid());
            exit(EXIT_SUCCESS); 
        }
    }

    // 3. PARENT WAITING FOR SIGNALS (Item 5 & 8)
    printf("Parent waiting for signals...\n");
    while(!sig_received) {
        // sigsuspend temporarily unblocks the mask and pauses.
        // When signal handler returns, sigsuspend returns.
        sigsuspend(&oldmask); 
    }
    printf("Signal received!\n");

    // 4. CLEANUP / WAIT FOR CHILDREN (Item 7)
    // Always clean up zombies
    while (wait(NULL) > 0); 
}
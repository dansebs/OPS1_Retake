#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global variable to store sender PID (or other data)
volatile sig_atomic_t last_sender_pid = 0;
volatile sig_atomic_t last_signal_code = 0;

// Advanced handler function signature
void sa_sigaction_handler(int sig, siginfo_t *info, void *ucontext) {
    last_signal_code = sig;
    last_sender_pid = info->si_pid; // This gets the PID of the sender
    // NOTE: Do not print here if possible. It's unsafe. 
    // If you MUST print for debugging, use write(), not printf().
}

// Setup function for SA_SIGINFO
void set_siginfo_handler(int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    
    act.sa_sigaction = sa_sigaction_handler;
    act.sa_flags = SA_SIGINFO; // Crucial: tells OS to pass extra info
    sigemptyset(&act.sa_mask);
    
    if (sigaction(sigNo, &act, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}
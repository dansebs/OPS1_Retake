// Requires: sigset_t mask, oldmask setup in main
// Requires: volatile sig_atomic_t last_signal global

printf("Process %d waiting for signals...\n", getpid());
while (1) {
    // 1. Wait for signal (unblocks temporarily)
    sigsuspend(&oldmask);

    // 2. Handler has run. Now check what happened.
    if (last_signal == SIGUSR1) {
        printf("Received SIGUSR1\n");
        last_signal = 0; // Reset flag
    }
    else if (last_signal == SIGINT) {
        printf("Received SIGINT. Cleaning up...\n");
        break; // Exit loop
    }
}
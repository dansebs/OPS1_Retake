// 1. Create a mask that blocks EVERYTHING
sigset_t block_all, wait_mask;
sigfillset(&block_all);
sigprocmask(SIG_BLOCK, &block_all, NULL);

// 2. Create a mask that allows ONLY the one signal we want (e.g., SIGUSR1)
sigfillset(&wait_mask);
sigdelset(&wait_mask, SIGUSR1); // Remove USR1 from the "Blocked" list

// 3. Wait
printf("Waiting strictly for SIGUSR1...\n");
while (last_signal != SIGUSR1) {
    sigsuspend(&wait_mask); // Will ONLY wake up for SIGUSR1
}
last_signal = 0;
printf("Got it!\n");
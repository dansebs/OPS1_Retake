sigset_t mask, oldmask;

// 1. Prepare a "Block Everything" set
sigfillset(&mask); 

// 2. Apply it (Block ALL signals)
sigprocmask(SIG_BLOCK, &mask, &oldmask);

// 3. Prepare a "Wait" set (All signals MINUS Sigusr1)
sigset_t wait_mask;
sigfillset(&wait_mask);
sigdelset(&wait_mask, SIGUSR1); // Unblock SIGUSR1 in this specific mask

// 4. Wait (suspend will swap the current mask with wait_mask)
while (last_signal != SIGUSR1) {
    sigsuspend(&wait_mask); 
}
// When sigsuspend returns, the "Block Everything" mask is automatically restored.

//Other option: Unblocking a specific signal only

sigset_t mask;

// 1. Add ALL signals to the set
sigfillset(&mask);

// 2. Remove SIGUSR1 from the "to-be-blocked" set
sigdelset(&mask, SIGUSR1);

// 3. Apply the mask to the process
// SIG_SETMASK overwrites the previous mask completely
if (sigprocmask(SIG_SETMASK, &mask, NULL) < 0)
    ERR("sigprocmask");
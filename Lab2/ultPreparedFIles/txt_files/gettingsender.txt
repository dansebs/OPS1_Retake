// 1. The Handler Function
void info_handler(int sig, siginfo_t *info, void *ctx) {
    // info->si_pid contains the PID of the sender
    printf("Received signal %d from PID %d\n", sig, info->si_pid);
    last_signal = sig;
    // OPTIONAL: Save PID to global if needed
    // last_sender_pid = info->si_pid; 
}

// 2. The Setup Function
void set_info_handler(int sig) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction)); // Zero out
    
    act.sa_sigaction = info_handler; // NOTE: .sa_sigaction, NOT .sa_handler
    act.sa_flags = SA_SIGINFO;       // NOTE: Critical flag
    
    sigemptyset(&act.sa_mask);
    if (sigaction(sig, &act, NULL) == -1) ERR("sigaction");
}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

// Terminates the process and kills the group if an error occurs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

// Simple handler setup
void set_handler(void (*f)(int), int sigNo) {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0; // Use SA_SIGINFO if you need sender PID
    act.sa_handler = f;
    if (sigaction(sigNo, &act, NULL) == -1) ERR("sigaction");
}
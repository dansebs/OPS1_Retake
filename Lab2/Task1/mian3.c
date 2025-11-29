#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t parent_count = 0;

void count_handler(int sig){
    parent_count++;
}

void set_handler(void (*f)(int), int sigNo) {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0; // Use SA_SIGINFO if you need sender PID
    act.sa_handler = f;
    if (sigaction(sigNo, &act, NULL) == -1) ERR("sigaction");
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s 0<n\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage(argv[0]);

    int n = atoi(argv[1]);
    if(n <= 0) ERR("Invalid n");

    // --- NEW PARENT SETUP ---
    set_handler(count_handler, SIGUSR1);
    //Block SIGUSR1 so we can wait for it safely
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK,&mask, &oldmask);




    // 2. CREATE CHILDREN (Item 6 & 10)
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork"); exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            srand(time(NULL) ^ getpid());
            //int num = min + rand() % (max - min + 1) logic for creating random numbers in a range
            int ms = 100 + rand() % 101;
            struct timespec t = {0, ms * 1000000L};

            for (int k = 0; k < 30; k++) {
                nanosleep(&t, NULL);       // Delay
                kill(getppid(), SIGUSR1);   // Send Signal
                write(1, "*", 1);           // Print *
            }
            
            exit(EXIT_SUCCESS); 
        }
    }

    // 3. PARENT WAITING FOR SIGNALS (Item 5 & 8)
     printf("[PARENT] Waiting for signals (Ctrl+C to stop)...\n");
     while(1){
        sigsuspend(&oldmask);
        printf("Count: %d\n",parent_count);
     }
    return EXIT_SUCCESS;

}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void handler(int sig){
    last_signal = sig;
}

int main(int argc, char** argv){

    if(argc != 2) ERR("USAGE: ./task3<n>");
    int n = atoi(argv[1]);
    if(n<0)ERR("Invalid n");

    pid_t *children = malloc(sizeof(pid_t) * n);

    if(!children) ERR("malloc");

    //Mask set up for parent
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = handler;
    sigaction(SIGUSR1,&act, NULL);

    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        if(pid < 0)ERR("fork");

        if(pid == 0){
            free(children);
            srand(time(NULL) ^ getpid());
            printf("[CHILD %d] Index %d. Waiting to start...\n", getpid(),i);

            while(last_signal != SIGUSR1){
                sigsuspend(&oldmask);
            }

            int counter = 0;
            printf("[CHILD %d] Activated!\n",getpid());

            while(1){
                int ms = 100 + rand() % 101;
                struct timespec ts = {0, ms * 1000000L};
                nanosleep(&ts,NULL);

                counter++;
                printf("{%d}: %d\n",getpid(), counter);
            }
            exit(EXIT_SUCCESS);
        }

        children[i] = pid;
    }
    
    printf("[PARENT] STARTING first child...\n");
    kill(children[0], SIGUSR1);


    while(wait(NULL) > 0);
    return EXIT_SUCCESS;

}
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

void child_smart_handler(int sig, siginfo_t *info, void *ctx) {
    if (sig == SIGUSR1) {
        if (info->si_pid == getppid()) {
            last_signal = sig; // Accepted
        } else {
            // Ignore signal from User/Group
        }
    } else {
        last_signal = sig; // Accept SIGUSR2/SIGINT from anyone
    }
}

int main(int argc, char** argv){
    
    if(argc != 2) ERR("USAGE: ./task3<n>");
    int n = atoi(argv[1]);
    if(n<0)ERR("Invalid n");

    setbuf(stdout, NULL);

    pid_t *children = malloc(sizeof(pid_t) * n);

    if(!children) ERR("malloc");

    //Mask set up for parent
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = handler;
    sigaction(SIGUSR1,&act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGINT,&act,NULL);

    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        if(pid < 0)ERR("fork");

        if(pid == 0){
            free(children);
            srand(time(NULL) ^ getpid());

            struct sigaction child_act;
            memset(&child_act, 0, sizeof(child_act));
            child_act.sa_sigaction = child_smart_handler;
            child_act.sa_flags = SA_SIGINFO;
            sigaction(SIGUSR1, &child_act, NULL);
            sigaction(SIGUSR2, &child_act, NULL);
            sigaction(SIGINT, &child_act, NULL);

            sigset_t empty_mask;
            sigemptyset(&empty_mask);
            sigprocmask(SIG_SETMASK, &empty_mask, NULL);

            sigset_t run_mask;
            sigemptyset(&run_mask);
            sigaddset(&run_mask, SIGUSR1); 


            int counter = 0;
            printf("[CHILD %d] Index %d. Waiting to start...\n", getpid(),i);
            while(1){

                sigprocmask(SIG_SETMASK, &empty_mask, NULL);

                while(last_signal != SIGUSR1){
                    if(last_signal == SIGINT) goto save_exit;
                    sigsuspend(&empty_mask);
                }
                last_signal = 0;

                
                printf("[CHILD %d] Activated!\n",getpid());

                sigprocmask(SIG_BLOCK, &run_mask, NULL);

                while(1){
                    int ms = 100 + rand() % 101;
                    struct timespec ts = {0, ms * 1000000L};
                    nanosleep(&ts,NULL);

                    if(last_signal == SIGUSR2){
                        printf("[CHILD %d] Stopping.\n",getpid());
                        last_signal = 0;
                        break;
                    }
                    if(last_signal == SIGINT) goto save_exit;

                    counter++;
                    printf("{%d}: %d\n",getpid(), counter);
                }
            }
        save_exit:
        
            {
                char fname[32]; sprintf(fname, "%d.txt", getpid());
                int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                if (fd >= 0) {
                    char buf[32]; 
                    int len = sprintf(buf, "%d", counter);
                    write(fd, buf, len);
                    close(fd);
                    printf("[CHILD %d] Saved %d to file.\n", getpid(), counter);
                }
            }
        
            exit(EXIT_SUCCESS);
        }

        children[i] = pid;
    }
    int current_child = 0;
    kill(children[0], SIGUSR1);

    while(1){
        sigsuspend(&oldmask);

        if(last_signal == SIGINT) {
            printf("[PARENT] Shutdown.\n");
            for(int k=0; k<n; k++) kill(children[k], SIGINT);
            break;
        }

        if(last_signal == SIGUSR1){
            kill(children[current_child],SIGUSR2);

            current_child = (current_child + 1 )% n;

            kill(children[current_child],SIGUSR1);

            printf("[PARENT] Switched to child %d\n",children[current_child]);
            last_signal = 0;
        }
    }

    while(wait(NULL) > 0);
    free(children);
    return EXIT_SUCCESS;

}
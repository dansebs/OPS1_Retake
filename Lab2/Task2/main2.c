#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

int main(int argc, char** argv){
    if(argc < 4){
        fprintf(stderr, "Usage: %s p t prob1 [prob2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int p = atoi(argv[1]);
    int t = atoi(argv[2]);
    int num_students =  argc - 3;

    if (p <= 0 || t <= 0) ERR("Invalid p or t");

    signal(SIGUSR1, SIG_IGN);

    printf("[TEACHER] Simulation started: %d students, %d parts, factor %d\n", num_students, p, t);

    

    for (int i = 0; i < num_students; i++)
    {
        int prob = atoi(argv[3 + i]);

        pid_t pid = fork();
        if(pid<0)ERR("fork");

        if(pid == 0){
            srand(time(NULL) ^ getpid());

            struct timespec work_time = {0, t * 100 * 1000000};
            printf("Student[%d,%d] has started doing task! probability of %d\n", i, getpid(), prob);

            for (int part = 1; part <= p; part++)
            {
                printf("Student[%d,%d] is starting doing part %d of %d\n", i, getpid(), part,p);

                nanosleep(&work_time, NULL);

                printf("Student[%d,%d] has finished part %d of %d\n", i, getpid(), part,p);
                if (kill(getppid(), SIGUSR1) < 0) ERR("kill");
            }
            

            printf("Student[%d, %d] has completed the task\n", i, getpid());
        
            exit(EXIT_SUCCESS);
        }
    }

    while(wait(NULL) != -1 || errno != ECHILD);

    printf("[TEACHER] All students left.\n");
    return EXIT_SUCCESS;
    
}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

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

    printf("[TEACHER] Simulation started: %d students, %d parts, factor %d\n", num_students, p, t);


    for (int i = 0; i < num_students; i++)
    {
        int prob = atoi(argv[3 + i]);

        pid_t pid = fork();
        if(pid<0)ERR("fork");

        if(pid == 0){
            printf("Student[%d, %d] ready with probability %d\n", i, getpid(), prob);
        
            exit(EXIT_SUCCESS);
        }
    }

    while(wait(NULL) != -1 || errno != ECHILD);

    printf("[TEACHER] All students left.\n");
    return EXIT_SUCCESS;
    
}
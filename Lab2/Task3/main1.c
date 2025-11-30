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

//Global variables that are strictly necessary
pid_t *children;
int n;

void cleanup_memory(){
    if(children) free(children);
}

int main(int argc, char** argv){

    if(argc != 2) ERR("USAGE: ./task3<n>");
    n = atoi(argv[1]);
    if(n<0)ERR("Invalid n");

    children = malloc(sizeof(pid_t) * n);

    if(!children) ERR("malloc");

    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        if(pid < 0)ERR("fork");

        if(pid == 0){
            free(children);
            printf("[CHILD %d] Index %d ready.\n", getpid(),i);
            exit(EXIT_SUCCESS);
        }

        children[i] = pid;
    }
    
    while(wait(NULL) > 0);

    cleanup_memory();
    printf("[PARENT] All children exited.\n");
    return EXIT_SUCCESS;

}
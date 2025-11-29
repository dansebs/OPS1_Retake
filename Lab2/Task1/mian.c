#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

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
            //struct timespec t = {0, ms * 1000000L};


            printf("Child %d (PID: %d) s = %d\n", i, getpid(), ms);
            exit(EXIT_SUCCESS); 
        }
    }

    // 3. PARENT WAITING FOR SIGNALS (Item 5 & 8)
    printf("All done.\n");
    return EXIT_SUCCESS;

}
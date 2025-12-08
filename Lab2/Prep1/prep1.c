#define _GNU_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void handler(int sig) {
    last_signal = sig;
}

// Helper to register handler (Mandatory for sigsuspend)
void set_handler(int sig, void (*f)(int)) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (sigaction(sig, &act, NULL) < 0) ERR("sigaction");
}


int open_file_write(const char *name) {
    // O_WRONLY: Write mode
    // O_CREAT: Create if missing
    // O_TRUNC: Empty the file if it exists
    // O_APPEND: (Optional) Add to end. Remove if you want to overwrite.
    // 0777: Permissions (rwx for everyone, umask will restrict this safely)
    int fd = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777));
    if (fd < 0) ERR("open");
    return fd;
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
    ssize_t c;
    size_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        
        if (c < 0) return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

int main(int argc, char **argv)
{
    if(argc != 4) ERR("Arguments too little");
    
    int nchld,rounds;
    char* file_name = argv[3];

    nchld = atoi(argv[1]);
    rounds = atoi(argv[2]);

    pid_t *children = malloc(sizeof(pid_t) * nchld);
    if(!children) ERR("malloc");

    int fd = open_file_write(file_name);
    bulk_write(fd,"0",1);
    close(fd);

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    set_handler(SIGUSR1, handler);
    set_handler(SIGINT, handler);

    for (int i = 0; i < nchld; i++)
    {
        pid_t pid = fork();
        if(pid < 0) ERR("fork");

        if(pid == 0){
            free(children);
            printf("CHILD %d\n",getpid());
            

            while(1){

                sigsuspend(&oldmask);

                if(last_signal == SIGINT) break;
                
                if(last_signal == SIGUSR1){
                    last_signal = 0;
                }
                
                kill(getppid(), SIGUSR1);
            }

            exit(EXIT_SUCCESS);
        }
        children[i] = pid;
    }

    for (int r = 0; r < rounds; r++) {
        for (int i = 0; i < nchld; i++) {
            // 1. Send to Child i
            kill(children[i], SIGUSR1);
            
            // 2. Wait for Child i to finish (it signals us back)
            while (last_signal != SIGUSR1) {
                sigsuspend(&oldmask);
            }
            last_signal = 0; // Reset
        }
        printf("[PARENT] Round %d complete.\n", r+1);
    }

    // Cleanup
    printf("[PARENT] Finished. Sending SIGINT.\n");
    for(int i=0; i<nchld; i++) kill(children[i], SIGINT);
    
    while(wait(NULL) != -1 || errno != ECHILD);
    free(children);
    unlink(file_name); // Cleanup file (Stage 4)
    
    return EXIT_SUCCESS;
    


    
}
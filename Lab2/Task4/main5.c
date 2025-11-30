#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL),exit(EXIT_FAILURE))

#define FILE_MAX_SIZE 512

volatile sig_atomic_t last_signal = 0;
// --- HELPER: Low-level Line Reader ---
// Reads from fd until newline, EOF, or buffer full.
ssize_t read_line(int fd, char* buf, size_t size) {
    ssize_t n = 0;
    char c;
    while ((size_t)n < size - 1) {
        // Read 1 byte at a time
        ssize_t res = TEMP_FAILURE_RETRY(read(fd, &c, 1));
        
        if (res < 0) return -1;     // Error
        if (res == 0) break;        // EOF
        if (c == '\n') break;       // End of line
        
        buf[n++] = c;
    }
    buf[n] = '\0'; // Null-terminate string
    return n;
}

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}


void usage(int argc, char* argv[])
{
    printf("%s p h\n", argv[0]);
    printf("\tp - path to directory describing the structure of the Austro-Hungarian office in Prague.\n");
    printf("\th - Name of the highest administrator.\n");
    exit(EXIT_FAILURE);
}

void handler(int sig) {
    last_signal = sig;
}
void handle_clerk(const char *base_path,const char *name, int is_root);

void run_child_process(const char *base_path, const char *name){
    handle_clerk(base_path, name, 0);
    exit(EXIT_SUCCESS);
}

void handle_clerk(const char *base_path, const char *name, int is_root) {
    // 1. Identity
    printf("My name is %s and my PID is %d\n", name, getpid());

    srand(time(NULL) ^ getpid());

    pid_t children[2] = {0,0};
    // 2. Construct Path (<dir>/<name>)
    char file_path[FILE_MAX_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", base_path, name);

    // 3. Open File (Low-Level)
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) ERR("open");

    char buf[128];
    // The file always contains exactly two lines.
    for (int i = 0; i < 2; i++) {
        // 4. Read Line
        if (read_line(fd, buf, sizeof(buf)) > 0) {
            // 5. Parse Content
            // Check if it is a real subordinate (not "-")
            if (strcmp(buf, "-") != 0) {
                printf("%s inspecting %s\n", name, buf);

                pid_t pid = fork();
                if(pid < 0)ERR("fork");

                if(pid==0){
                    close(fd);

                    run_child_process(base_path,buf);
                }
                children[i] = pid;
            }
        }
    }

    close(fd);

    // 6. Completion
    printf("%s has inspected all subordinates\n", name);

    sigset_t wait_mask;
    sigemptyset(&wait_mask);

    while(1){
        sigsuspend(&wait_mask);

        int current_sig = last_signal;
        last_signal = 0;

        if (current_sig==SIGUSR2)
        {

            if(rand() % 3 == 0){
                printf("%s received a document. Sending to the archive,\n",name);
            }else{
                if(is_root){
                    printf("%s received a document. Ignoring.\n",name);
                }else{
                    printf("%s received a document. Passing on to the superintendent,\n",name);
                    kill(getppid(),SIGUSR2);
                }
            }
        }
        
        //Note: stage 5
        if (current_sig == SIGINT) {
            printf("%s ending the day.\n", name);
            
            // 1. Forward Signal to Children
            for (int i = 0; i < 2; i++) {
                if (children[i] > 0) {
                    kill(children[i], SIGINT);
                }
            }

            // 2. Wait for them to finish
            for (int i = 0; i < 2; i++) {
                if (children[i] > 0) {
                    waitpid(children[i], NULL, 0);
                }
            }

            // 3. Leave
            printf("%s is leaving the office\n", name);
            exit(EXIT_SUCCESS);
        }
    }

}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

int main(int argc, char* argv[]) {
    if (argc != 3){
        usage(argc,argv);
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    sigaddset(&mask,SIGUSR2);
    sigaddset(&mask,SIGINT);

    sigprocmask(SIG_BLOCK, &mask, NULL);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigaction(SIGINT,&act,NULL);
    sigaction(SIGUSR1,&act,NULL);
    sigaction(SIGUSR2,&act,NULL);

    handle_clerk(argv[1], argv[2],1);

    return EXIT_SUCCESS;

}
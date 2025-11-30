#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL),exit(EXIT_FAILURE))

#define FILE_MAX_SIZE 512

// --- HELPER: Low-level Line Reader ---
// Reads from fd until newline, EOF, or buffer full.
ssize_t read_line(int fd, char* buf, size_t size) {
    ssize_t n = 0;
    char c;
    while (n < size - 1) {
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
    // Do nothing
}

void handle_clerk(const char *base_path, const char *name) {
    // 1. Identity
    printf("My name is %s and my PID is %d\n", name, getpid());

    // 2. Construct Path (<dir>/<name>)
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", base_path, name);

    // 3. Open File (Low-Level)
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) ERR("open");

    char buf[128];
    pid_t children[2] = {0,0};
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

                    handle_clerk(base_path, buf);

                    exit(EXIT_SUCCESS);
                }

                children[i] = pid;
            }
        }
    }

    close(fd);

    // 6. Completion
    printf("%s has inspected all subordinates\n", name);

    for (int i = 0; i < 2; i++)
    {
        if(children[i] > 0){
            waitpid(children[i], NULL, 0);
        }
    }
    
    printf("%s is leaving the office\n",name);
    // 7. Termination
    exit(EXIT_SUCCESS);
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

    if(sigprocmask(SIG_BLOCK, &mask, NULL)<0)ERR("sigprocmask");


    handle_clerk(argv[1], argv[2]);

    printf("Received SIGUSR1. Terminating.\n");
    return EXIT_SUCCESS;

}
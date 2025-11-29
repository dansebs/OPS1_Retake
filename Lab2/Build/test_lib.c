#include "mylib.h"
#include <sys/wait.h>

int main(int argc, char** argv) {
    // --- TEST 1: FILE I/O ---
    printf("--- Phase 1: Testing File I/O ---\n");
    
    // 1. Create a file and write to it using bulk_write
    int fd = open("test.txt", O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if (fd < 0) ERR("open write");
    
    char *content = "Line 1: Hello\nLine 2: World\nLine 3: Operating Systems\n";
    // We calculate length manually or use strlen if <string.h> is included
    int len = 0; while(content[len]) len++; 
    
    if (bulk_write(fd, content, len) < 0) ERR("bulk_write");
    close(fd);
    
    // 2. Read it back line-by-line using read_line
    fd = open("test.txt", O_RDONLY);
    if (fd < 0) ERR("open read");
    
    char buf[128];
    printf("Reading file content:\n");
    while (read_line(fd, buf, sizeof(buf)) > 0) {
        printf("  [READ]: %s", buf); // buf already has \n
    }
    close(fd);

    // --- TEST 2: SIGNALS ---
    printf("\n--- Phase 2: Testing Signals ---\n");

    // 1. Setup Handler
    set_siginfo_handler(SIGUSR1);

    // 2. Mask signals (Block them so we can wait safely)
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    // 3. Fork
    pid_t pid = fork();
    if (pid < 0) ERR("fork");

    if (pid == 0) {
        // CHILD
        printf("  [CHILD] PID %d created. Sleeping 1s...\n", getpid());
        sleep(1);
        printf("  [CHILD] Sending SIGUSR1 to parent.\n");
        kill(getppid(), SIGUSR1);
        exit(EXIT_SUCCESS);
    }

    // PARENT
    printf("  [PARENT] Waiting for signal...\n");
    // This pauses until a signal in 'mask' arrives
    sigsuspend(&oldmask); 

    // When we wake up, the handler has already run
    if (last_signal_code == SIGUSR1) {
        printf("  [PARENT] SUCCESS! Received SIGUSR1 from PID %d\n", last_sender_pid);
    } else {
        printf("  [PARENT] FAILURE! Received wrong signal.\n");
    }

    // Clean up child
    wait(NULL);
    printf("--- Test Complete ---\n");
    return EXIT_SUCCESS;
}
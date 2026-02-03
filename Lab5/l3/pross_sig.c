#define _GNU_SOURCE // Required for TEMP_FAILURE_RETRY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include "macros.h" // Your L0 macros

// ==========================================
// 🔧 SECTION 1: SIGNAL MANAGEMENT
// ==========================================

/* Sets a simple handler (no sender info) */
void set_handler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    // SA_RESTART: Automatically restart interrupted primitives (read/write)
    // REMOVE this flag if you want to handle EINTR manually or use sigsuspend logic
    act.sa_flags = 0; 
    sigemptyset(&act.sa_mask);
    if (sigaction(sigNo, &act, NULL) == -1) ERR("sigaction");
}

/* Sets a handler that can identify the sender PID */
void set_info_handler(void (*f)(int, siginfo_t*, void*), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO; // Critical: tells OS to use sa_sigaction
    sigemptyset(&act.sa_mask);
    if (sigaction(sigNo, &act, NULL) == -1) ERR("sigaction");
}

/* Helper to block signals before a critical section */
void block_signal(int sigNo, sigset_t *oldmask) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sigNo);
    if (sigprocmask(SIG_BLOCK, &mask, oldmask) == -1) ERR("sigprocmask block");
}

/* Helper to unblock signals */
void unblock_signal(int sigNo) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sigNo);
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) ERR("sigprocmask unblock");
}

// ==========================================
// 👶 SECTION 2: PROCESSES
// ==========================================

/* Creates a child. Returns PID to parent, 0 to child. 
 * Handles error checking automatically. */
pid_t safe_fork() {
    pid_t pid = fork();
    if (pid < 0) ERR("fork");
    return pid;
}

/* Waits for ALL children to finish. 
 * Essential to prevent Zombies. */
void wait_for_all_children() {
    while (wait(NULL) > 0); 
    // ECHILD is normal here (no more children), other errors are bad
    if (errno != ECHILD) ERR("wait"); 
}

/* Non-blocking wait (Zombie reaper for signal handlers) */
void reap_zombies() {
    pid_t pid;
    // WNOHANG: Don't block if child is still running
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        // Optional: printf("Reaped child %d\n", pid);
    }
}

// ==========================================
// 💾 SECTION 3: LOW-LEVEL I/O (ROBUST)
// ==========================================

/* Reads exactly 'count' bytes, handling interrupts and partial reads */
ssize_t bulk_read(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;
    do {
        // TEMP_FAILURE_RETRY is a GNU macro that re-calls read if it sets errno=EINTR
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c;
        if (c == 0) return len; // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

/* Writes exactly 'count' bytes */
ssize_t bulk_write(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

/* Safe sleep (handles signal interruptions) */
void safe_nanosleep(long sec, long nsec) {
    struct timespec req = {sec, nsec};
    struct timespec rem = {0, 0};
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) req = rem; // Resume with remaining time
        else ERR("nanosleep");
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

// Terminates the process and kills the group if an error occurs
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))


// Reads exactly 'count' bytes from fd into buf
ssize_t bulk_read(int fd, char *buf, size_t count) {
    ssize_t c;
    size_t len = 0;
    do {
        c = read(fd, buf, count);
        if (c < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            return -1; // Real error
        }
        if (c == 0) return len; // EOF reached
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

// Writes exactly 'count' bytes from buf to fd
ssize_t bulk_write(int fd, char *buf, size_t count) {
    ssize_t c;
    size_t len = 0;
    do {
        c = write(fd, buf, count);
        if (c < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}
#define _GNU_SOURCE // Required for TEMP_FAILURE_RETRY
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

ssize_t bulk_read(int fd, char *buf, size_t count) {
    ssize_t c;
    size_t len = 0;
    do {
        // The macro handles the EINTR loop for us!
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        
        if (c < 0) return c; // Real error (not EINTR)
        if (c == 0) return len; // EOF
        
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
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
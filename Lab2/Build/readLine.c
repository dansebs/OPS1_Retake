#include <unistd.h>
#include <errno.h>

/* 
 * Reads a line from fd into buffer 'buf' of size 'size'.
 * Returns: number of bytes read (including newline), or -1 on error.
 * Stops at newline, EOF, or when buffer is full.
 * Ensures the string is null-terminated.
 */
ssize_t read_line(int fd, char *buf, size_t size) {
    ssize_t n = 0;
    char c;
    while (n < size - 1) {
        ssize_t res = read(fd, &c, 1);
        
        if (res == -1) {
            if (errno == EINTR) continue; // Interrupted, try again
            return -1;
        }
        if (res == 0) break; // EOF

        buf[n++] = c;
        if (c == '\n') break;
    }
    buf[n] = '\0'; // Null-terminate
    return n;
}
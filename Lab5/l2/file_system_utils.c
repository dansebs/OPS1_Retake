#define _GNU_SOURCE // Required for TEMP_FAILURE_RETRY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>      // Low-level: open flags
#include <unistd.h>     // Low-level: close, read, write, unlink
#include <sys/stat.h>   // chmod, fchmod, macros
#include "macros.h"     // Your macros from L0

// ==========================================
// 🔧 SECTION 1: LOW-LEVEL HELPERS (POSIX)
// ==========================================

/* * bulk_read: Handles partial reads and interrupts (EINTR).
 * Crucial because 'read' may return fewer bytes than requested.
 */
ssize_t bulk_read(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;
    do {
        // TEMP_FAILURE_RETRY retries the call if interrupted by a signal (EINTR)
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c; // Error
        if (c == 0) return len; // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

/* * bulk_write: Handles partial writes and interrupts.
 * Ensures ALL data is pushed to the disk descriptor.
 */
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

/* Low-Level: Create or Replace file */
int create_file_low(const char *path, mode_t mode) {
    // O_TRUNC: clears file if exists. O_CREAT: creates if missing.
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd == -1) ERR("open (low-level)");
    return fd;
}

/* Low-Level: Change permissions using File Descriptor */
void chmod_low(int fd, mode_t mode) {
    if (fchmod(fd, mode) == -1) ERR("fchmod");
}

/* Low-Level: Read entire contents into buffer */
ssize_t read_file_low(const char *path, char *buffer, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) ERR("open reading");
    
    ssize_t total_read = bulk_read(fd, buffer, size);
    if (total_read == -1) ERR("bulk_read");
    
    if (close(fd) == -1) ERR("close");
    return total_read;
}

// ==========================================
// 📚 SECTION 2: HIGH-LEVEL HELPERS (STDIO)
// ==========================================

/* High-Level: Create or Replace file */
FILE* create_file_high(const char *path) {
    // "w+" creates empty file or truncates existing one
    FILE *fp = fopen(path, "w+");
    if (!fp) ERR("fopen");
    return fp;
}

/* High-Level: Change permissions (Path based) */
void chmod_high(const char *path, mode_t mode) {
    // Standard C doesn't have 'fchmod' for FILE*, so we use chmod on the path
    if (chmod(path, mode) == -1) ERR("chmod");
}

/* High-Level: Read contents */
size_t read_file_high(const char *path, char *buffer, size_t size) {
    FILE *fp = fopen(path, "r");
    if (!fp) ERR("fopen");

    // fread returns number of *elements* read
    size_t bytes_read = fread(buffer, 1, size, fp);
    
    if (ferror(fp)) ERR("fread");
    if (fclose(fp)) ERR("fclose");
    
    return bytes_read;
}
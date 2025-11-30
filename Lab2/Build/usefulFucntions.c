#include <time.h>

// Usage: msleep(500); // Sleep 500 milliseconds
void msleep(int ms) {
    struct timespec t;
    t.tv_sec = ms / 1000;             // Seconds
    t.tv_nsec = (ms % 1000) * 1000000L; // Nanoseconds
    
    // We ignore the remaining time if interrupted (NULL)
    // If you need strict timing even with signals, use a while loop here.
    nanosleep(&t, NULL); 
}
// Usage: char *buf = safe_malloc(size);
char *safe_malloc(size_t size) {
    char *buf = malloc(size);
    if (!buf) {
        ERR("malloc"); // Uses your ERR macro to exit immediately
    }
    return buf;
}

#include <fcntl.h>

// Usage: int fd = open_file_write("output.txt");
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
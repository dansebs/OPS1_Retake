#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

/* Context for file editing */
typedef struct {
    int fd;     // File descriptor
    int items;  // Number of items to write
} edit_ctx_t;

/* * Thread Function: Writes to a specific slot in the file 
 * This allows N threads to write to the same file simultaneously without a Mutex!
 * Requirement: The file must be pre-allocated or you must know the offsets.
 */
void* thread_edit_func(void* arg) {
    int id = *(int*)arg;
    edit_ctx_t *ctx = (edit_ctx_t*)((void**)arg)[1]; // extracting ctx from packed args if needed
    
    char buffer[64];
    int len = snprintf(buffer, 64, "Thread %d was here\n", id);

    /* CALCULATE OFFSET:
       If every record is fixed size (e.g., 64 bytes), thread K writes to K * 64. */
    off_t offset = id * 64; 

    /* pwrite(fd, buffer, length, offset)
       This writes to 'offset' regardless of where other threads are. 
       It is THREAD-SAFE for disjoint regions. */
    if (pwrite(ctx->fd, buffer, len, offset) != len) {
        perror("pwrite");
    }

    return NULL;
}

/* Setup Function */
void run_file_edit_demo() {
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    edit_ctx_t ctx = {fd, 10};
    
    /* ... create threads passing &ctx ... */
    
    close(fd);
}
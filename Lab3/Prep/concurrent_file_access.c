#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define ERR(source) (perror(source), exit(EXIT_FAILURE))

typedef struct {
    int fd;
    int chunk_size;
} read_ctx_t;

typedef struct {
    read_ctx_t *ctx;
    int chunk_index; // Thread 0 reads chunk 0, Thread 1 reads chunk 1...
} args_t;

/* * Thread Function: Reads a specific chunk independently
 */
void* thread_read_func(void* arg) {
    args_t *a = (args_t*)arg;
    
    char *buf = malloc(a->ctx->chunk_size);
    if(!buf) return NULL;

    off_t offset = a->chunk_index * a->ctx->chunk_size;

    /* pread(fd, buffer, size, offset)
       Reads safely from 'offset'. No mutex needed! 
       Allows true parallel reading. */
    ssize_t bytes_read = pread(a->ctx->fd, buf, a->ctx->chunk_size, offset);

    if (bytes_read > 0) {
        printf("Thread %d read: %.*s\n", a->chunk_index, (int)bytes_read, buf);
    } else if (bytes_read == 0) {
        printf("Thread %d hit EOF\n", a->chunk_index);
    } else {
        perror("pread");
    }

    free(buf);
    return NULL;
}

void run_file_read_demo(char *filename, int n_threads) {
    int fd = open(filename, O_RDONLY);
    if(fd < 0) ERR("open");

    read_ctx_t ctx = {fd, 128}; // Each thread reads 128 bytes

    pthread_t *tids = malloc(n_threads * sizeof(pthread_t));
    args_t *args = malloc(n_threads * sizeof(args_t));

    for(int i = 0; i < n_threads; i++) {
        args[i].ctx = &ctx;
        args[i].chunk_index = i;
        pthread_create(&tids[i], NULL, thread_read_func, &args[i]);
    }

    for(int i = 0; i < n_threads; i++) pthread_join(tids[i], NULL);

    free(tids);
    free(args);
    close(fd);
}
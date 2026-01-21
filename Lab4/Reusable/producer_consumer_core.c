// producer_consumer_core.c
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    void (*function)(void *); // Function pointer for the task
    void *arg;                // Argument for the function
} task_t;

typedef struct {
    task_t *buffer;           // Circular buffer or single slot
    int capacity;
    int count;
    
    // Synchronization
    pthread_mutex_t lock;     // Protects the buffer
    pthread_cond_t work_ready;// Signals workers: "New task arrived!"
    sem_t empty_slots;        // Signals producer: "Space available!"
    
    int shutdown;             // Shutdown flag
} thread_pool_t;

// WORKER THREAD LOGIC (Consumer)
void* worker_routine(void *arg) {
    thread_pool_t *pool = (thread_pool_t*)arg;
    
    while(1) {
        pthread_mutex_lock(&pool->lock);
        
        // WAIT until work is available or shutdown is triggered
        while (pool->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->work_ready, &pool->lock);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break; // Exit loop
        }

        // GRAB TASK
        task_t task = pool->buffer[--pool->count]; // Simplified stack logic
        
        pthread_mutex_unlock(&pool->lock);
        sem_post(&pool->empty_slots); // Notify producer: "Slot opened!"

        // EXECUTE TASK
        // (This runs outside the lock to allow concurrency!)
        task.function(task.arg);
    }
    return NULL;
}
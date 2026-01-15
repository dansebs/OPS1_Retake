#include <pthread.h>

/* * TEMPLATE A: The "Array Work" Context
 * Use for: Task 1 (Sqrt), Task 2 (Swapper), Task 3 (Race)
 */
typedef struct {
    int *array;                 // The shared data array
    pthread_mutex_t *mx_array;  // Mutexes for the array
    int n;                      // Size of array
    int active_workers;         // For limiting threads
    pthread_mutex_t mx_stats;   // Protects active_workers
} array_context_t;

/* * TEMPLATE B: The "File Work" Context
 * Use for: Reading/Writing files with multiple threads
 */
typedef struct {
    int fd;                     // File Descriptor (from open())
    char *filename;
    pthread_mutex_t mx_file;    // Protects file offsets if using write/read
    int records_processed;
    pthread_mutex_t mx_stats;
} file_context_t;

/* * HOW TO PASS TO THREAD:
 * Usually, a thread needs the context AND its own specific ID/Seed.
 * We create a wrapper struct for that.
 */
typedef struct {
    array_context_t *app_ctx;   // Pointer to the big shared struct
    int thread_id;              // Unique ID for this thread
    unsigned int seed;          // Random seed for rand_r
    int job_start_index;        // Optional: specific chunk to work on
} thread_args_t;
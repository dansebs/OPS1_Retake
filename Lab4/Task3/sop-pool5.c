#include "header.h"

#define MAX_POOL_SIZE 16

typedef struct thread_pool
{
    pthread_t *threads;
    int num_threads;

    void(*work_function)(void *);
    void *work_args;

    pthread_mutex_t lock;
    pthread_cond_t cond_work_ready;
    pthread_cond_t cond_task_received;

    sem_t available_slots; // Semaphore added to track free workers

    int has_work;
    int shutdown;
} thread_pool_t;


void *worker_thread(void *args) {
    
    thread_pool_t *pool = (thread_pool_t *) args;

    while(1){
        pthread_mutex_lock(&pool->lock);

        while(pool->has_work == 0 && !pool->shutdown){
            pthread_cond_wait(&pool->cond_work_ready, &pool->lock);
        }

        if(pool->shutdown && pool->has_work == 0){
            pthread_mutex_unlock(&pool->lock);
            return NULL;
        }

        void (*func)(void *) = pool->work_function;
        void *arg = pool->work_args;

        pool->has_work = 0;
        pthread_cond_broadcast(&pool->cond_task_received);

        pthread_mutex_unlock(&pool->lock);

        if(func != NULL){
            func(arg);
        }

        // Signal that the worker is now free after finishing the function
        sem_post(&pool->available_slots);
    }

    return NULL;
}

thread_pool_t *initialize(int N)
{
    if (N > MAX_POOL_SIZE)
        ERR("thread pool is too big!");

    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (!pool) ERR("malloc");

    pool->num_threads = N;
    pool->has_work = 0;
    pool->shutdown = 0;
    pool->work_function = NULL;
    pool->work_args = NULL;

    // Initialize semaphore with N available slots
    if (sem_init(&pool->available_slots, 0, N) != 0) ERR("sem_init");

    pool->threads = malloc(N * sizeof(pthread_t));
    if (!pool->threads) ERR("malloc");

    if (pthread_mutex_init(&pool->lock, NULL) != 0) ERR("pthread_mutex_init");
    if (pthread_cond_init(&pool->cond_work_ready, NULL) != 0) ERR("pthread_cond_init");
    if (pthread_cond_init(&pool->cond_task_received, NULL) != 0) ERR("pthread_cond_init");

    for (int i = 0; i < N; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, (void *)pool) != 0)
            ERR("pthread_create");
    }

    return pool;
}

void dispatch(thread_pool_t *pool, void (*work_function)(void *), void *args) 
{
    // Wait for an available worker slot before occupying the mailbox
    TEMP_FAILURE_RETRY(sem_wait(&pool->available_slots));

    pthread_mutex_lock(&pool->lock);

    while(pool->has_work == 1){
        pthread_cond_wait(&pool->cond_task_received, &pool->lock);
    }

    pool->work_function = work_function;
    pool->work_args = args;
    pool->has_work = 1;

    pthread_cond_signal(&pool->cond_work_ready);

    pthread_mutex_unlock(&pool->lock);
}

void cleanup(thread_pool_t *pool) 
{
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->cond_work_ready);
    pthread_mutex_unlock(&pool->lock);

    for(int i = 0; i < pool->num_threads; i++){
        pthread_join(pool->threads[i], NULL);
    }
    
    sem_destroy(&pool->available_slots);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond_work_ready);
    pthread_cond_destroy(&pool->cond_task_received);

    free(pool->threads);
    free(pool);

    printf("cleanup\n"); 
}

/**
 * Worker functions
 */
typedef struct monte_carlo_args
{
    float radius;
    unsigned int sample_count;
    unsigned int hit_count;
    unsigned int seed;
    pthread_barrier_t *barrier;
} monte_carlo_args_t;

typedef struct monte_carlo_args_array
{
    monte_carlo_args_t *args;
    int thread_count;
    int task_idx;
    float radius;
    pthread_barrier_t barrier;
} monte_carlo_args_array_t;

void circle_monte_carlo(void *args)
{
    monte_carlo_args_t *mc_args = (monte_carlo_args_t *)args;

    for (unsigned int i = 0; i < mc_args->sample_count; ++i)
    {
        double rand_x = (double)rand_r(&(mc_args->seed)) / RAND_MAX;
        double rand_y = (double)rand_r(&(mc_args->seed)) / RAND_MAX;

        if (rand_x * rand_x + rand_y * rand_y <= 1.0)
            mc_args->hit_count++;
        sleep_ms();
    }
    pthread_barrier_wait(mc_args->barrier);
}

void accumulate_monte_carlo(void *args)
{
    monte_carlo_args_array_t *mc_args = (monte_carlo_args_array_t *)args;

    pthread_barrier_wait(&mc_args->barrier);
    unsigned int hit_total = 0, samples_total = 0;
    for (int i = 0; i < mc_args->thread_count; i++)
    {
        hit_total += mc_args->args[i].hit_count;
        samples_total += mc_args->args[i].sample_count;
    }

    double res = 4.0 * mc_args->radius * mc_args->radius * (double)hit_total / samples_total;
    printf("TASK %d, Circle area of radius %f result %lf\n", mc_args->task_idx, mc_args->radius, res);

    pthread_barrier_destroy(&mc_args->barrier);
    free(mc_args->args);
    free(mc_args);
}

void start_monte_carlo(thread_pool_t *pool, int sampling_worker_count, float circle_radius, unsigned int sample_count,
                       int task_idx)
{

    if (sampling_worker_count >= pool->num_threads) {
        sampling_worker_count = pool->num_threads - 1; 
    }else{
        sampling_worker_count--;
    }
    if (sampling_worker_count < 1) sampling_worker_count = 1;

    printf("Starting TASK %d: calculating area of circle with radius %.2f\n", task_idx, circle_radius);

    monte_carlo_args_array_t *args = malloc(sizeof(monte_carlo_args_array_t));
    if (!args) ERR("malloc");
    args->args = malloc(sizeof(monte_carlo_args_t) * sampling_worker_count);
    if (!args->args) ERR("malloc");
    
    args->thread_count = sampling_worker_count;
    args->radius = circle_radius;
    args->task_idx = task_idx;

    pthread_barrier_init(&args->barrier, NULL, sampling_worker_count + 1);

    for (int i = 0; i < sampling_worker_count; ++i)
    {
        args->args[i].radius = circle_radius;
        args->args[i].sample_count = sample_count / sampling_worker_count;
        args->args[i].hit_count = 0;
        args->args[i].seed = rand();
        args->args[i].barrier = &args->barrier;

        dispatch(pool, circle_monte_carlo, &(args->args[i]));
    }

    dispatch(pool, accumulate_monte_carlo, args);
}

void start_hello_work(thread_pool_t *pool, int sampling_worker_count)
{
    for (int i = 0; i < sampling_worker_count; ++i)
    {
        int* number = malloc(sizeof(int));
        if (!number) ERR("malloc");
        *number = i;
        dispatch(pool, hello_world_test, number);
    }
}

void parse_monte_carlo(thread_pool_t *pool, int worker_count, int task_idx)
{
    float radius = read_float_cli();
    if (radius < 0) {
        fprintf(stderr, "Invalid radius\n");
        return;
    }

    int sample_count = read_int_cli();
    if (sample_count < worker_count) {
        fprintf(stderr, "Invalid sample count\n");
        return;
    }

    start_monte_carlo(pool, worker_count, radius, sample_count, task_idx);
}

int parse_cli(thread_pool_t *pool)
{
    int number = read_int_cli();
    if (number < 1 || number > 3) {
        fprintf(stderr, "Invalid command\n");
        return 1;
    } else if (number == 3) {
        return 0;
    }

    int worker_count = read_int_cli();
    if (worker_count < 1 || worker_count > MAX_POOL_SIZE) {
        fprintf(stderr, "Invalid worker_count\n");
        return 1;
    }

    static int task_idx = 0;
    task_idx++;

    switch (number)
    {
        case 1: parse_monte_carlo(pool, worker_count, task_idx); break;
        case 2: start_hello_work(pool, worker_count); break;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    srand(4);

    if (argc != 2) {
        printf("%s <N>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int pool_size = atoi(argv[1]);
    if (pool_size <= 0) {
        printf("Invalid thread count");
        exit(EXIT_FAILURE);
    }

    thread_pool_t *pool = initialize(pool_size);

    do {
        printf("\nenter command\n");
        printf("1. circle <n> <r> <s>\n");
        printf("2. hello <n>\n");
        printf("3. exit\n\n");
    } while (parse_cli(pool));

    cleanup(pool);
    return EXIT_SUCCESS;
}
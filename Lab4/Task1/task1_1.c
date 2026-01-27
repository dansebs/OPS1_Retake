#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ERR(source) (perror(source),fprintf(stderr, "%s:%d\n",__FILE__, __LINE__), exit(EXIT_FAILURE))

#define MAXLINE 120

typedef struct{
    long offset;
    long size;
}task;

typedef struct{
    task *tsk;
    int total_tasks;
    int *next_task_idx;
    pthread_mutex_t *task_mutex;
}thread_arg_t;

void* thread_func(void *void_arg)
{
    thread_arg_t *arg = (thread_arg_t*)void_arg;
    while(1)
    { 
        pthread_mutex_lock(arg->task_mutex);
        if(*(arg->next_task_idx)>= arg->total_tasks){
            pthread_mutex_unlock(arg->task_mutex);
            break;
        }
        int current_idx = *(arg->next_task_idx);
       ( *(arg->next_task_idx))++;
        long a = arg->tsk[current_idx].offset;
        long b = arg->tsk[current_idx].size;

        pthread_mutex_unlock(arg->task_mutex);
        printf("Size: %ld Offset: %ld\n",a,b);
    }

    return NULL;
}
int main(int argc,char** argv)
{
    if(argc < 4){
        ERR("usage  n m file_name");
    }
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char* file_name = argv[3];

    char *header = NULL;
    size_t len = 0;
    ssize_t nread;


    
    long file_size = 0;
    int fd;
    fd = open(file_name, O_RDONLY|O_CREAT);

    nread = getline(&header, &len, fptr);
    //getdelim can go until the specified delimiter, getline is until new line
    long header_end = ftell(fptr);
    printf("Header is: %s\n size of header is length %ld\n",header,nread);
    fseek(fptr,0L,SEEK_END);
    file_size = ftell(fptr);
    long data_size = file_size - header_end;
    long chunk_size = data_size / m;
    printf("The total size of the file is: %ld\n The size of the data is %ld \n The chunk size is %ld\n", file_size,data_size,chunk_size);


    pthread_t tids[n];
    task tasks[m];
    for(int i = 0; i < m; i++){
        tasks[i].offset = header_end + i * chunk_size;
        tasks[i].size = chunk_size;
    }
    tasks[m-1].size += data_size % m;
    pthread_mutex_t mx_global = PTHREAD_MUTEX_INITIALIZER;
    thread_arg_t args[n];
    int next_task_idx = 0;
    for (int i = 0; i < n; i++)
    {
        args[i].tsk = tasks;
        args[i].total_tasks = m;
        args[i].next_task_idx = &next_task_idx;
        args[i].task_mutex = &mx_global;
    }

   
    for (int i = 0; i < n; i++)
    {
        if(pthread_create(&tids[i], NULL, thread_func, &args[i])) 
            ERR("pthread_create");
    }
    
    for(int i = 0; i < n; i++) {
        if(pthread_join(tids[i], NULL)) ERR("pthread_join");
    }
    fclose(fptr);
    free(header);
    return EXIT_SUCCESS;
}
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
#define UNUSED(x) (void)(x)

typedef struct RingBuffer{

    char* rf_str;
    
}RingBuffer;

int main(int argc,char** argv)
{
    if(argc < 6){
        ERR("usage  n m file_name");
    }
    int n_consumers = atoi(argv[1]);
    int buffer_size = atoi(argv[2]);
    char* input_file = argv[3];
    char* output_file_1 = argv[4];
    char* output_file_2 = argv[5];

    

    FILE *fptr;
    fptr = fopen(input_file, "r");
    if(fptr == NULL) ERR("fopen");

    FILE *fptr_o1;
    FILE *fptr_o2;
    fptr_o1 =fopen(output_file_1,"w");
    fptr_o2 =fopen(output_file_2,"w");
    
    UNUSED(fptr_o1);
    UNUSED(fptr_o2);
    UNUSED(buffer_size);
    UNUSED(n_consumers);


}   
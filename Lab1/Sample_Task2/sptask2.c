#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_P_ARGS 100


int main(int argc, char **argv)
{
    int c;

    char *p_values[MAX_P_ARGS];
    int p_count = 0;
    long d_value = 0;
    char *e_value = NULL;
    int o_value = 0;
    while ((c = getopt(argc, argv, "p:d:e:o")) != -1)
    {
            switch (c)
            {
                case 'p':
                    if(p_count < MAX_P_ARGS){
                        p_values[p_count++] = optarg;
                        
                    }else{
                        fprintf(stderr, "Too many -p arguments!\n");
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'd':
                    d_value = atoi(optarg);
                    
                    break;
                case 'e':
                    e_value = optarg;
                    
                    break;
                case 'o':
                    o_value = 1;
                    
                case '?':
                default:
                    //usage(argv[0]);
            }
    }
    for (int i = 0; i < p_count; i++)
    {
        printf("%s %ld %s %d\n", p_values[i],d_value,e_value,o_value);
    }
    
    
    
}
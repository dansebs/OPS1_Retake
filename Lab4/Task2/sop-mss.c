#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define UNUSED(x) ((void)(x))

#define DECK_SIZE (4 * 13)
#define HAND_SIZE (7)

void usage(const char *program_name)
{
    fprintf(stderr, "USAGE: %s n\n", program_name);
    exit(EXIT_FAILURE);
}
typedef struct thread_arg_t {
    int hand[HAND_SIZE];
    int id;
}thread_arg_t;
void shuffle(int *array, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void print_deck(const int *deck, int size)
{
    const char *suits[] = {" of Hearts", " of Diamonds", " of Clubs", " of Spades"};
    const char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};

    char buffer[1024];
    int offset = 0;

    if (size < 1 || size > DECK_SIZE)
        return;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[");
    for (int i = 0; i < size; ++i)
    {
        int card = deck[i];
        if (card < 0 || card > DECK_SIZE)
            return;
        int suit = deck[i] % 4;
        int value = deck[i] / 4;

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", values[value]);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", suits[suit]);
        if (i < size - 1)
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, ", ");
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "]");

    puts(buffer);
}

void *thread_func(void *void_arg ){

    thread_arg_t *arg = (thread_arg_t*)void_arg;
    printf("Player %d\n",arg->id);
    print_deck(arg->hand,HAND_SIZE);
    printf("\n");
    return NULL;
}
int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    srand(time(NULL));

    int deck[DECK_SIZE];
    for (int i = 0; i < DECK_SIZE; ++i)
        deck[i] = i;
    shuffle(deck, DECK_SIZE);
    //print_deck(deck, DECK_SIZE);

    if(argc != 2){
        usage("sop-mss");
    }

    int n = atoi(argv[1]);

    if(n < 4 || n > 7){
        usage("n < 4 or n > 7");
    }

    thread_arg_t args[n];

    for(int i = 0; i < n; i++){
        for(int j = 0; j < HAND_SIZE; j++){
            args[i].hand[j] = deck[j + (i * HAND_SIZE)];
        }
        args[i].id = i;
    }


    pthread_t tids[n];
    for(int i = 0; i < n; i++){
        pthread_create(&tids[i],NULL,thread_func,&args[i]);
    }

    for(int i = 0; i < n; i++) {
        if(pthread_join(tids[i], NULL)) ERR("pthread_join");
    }

    exit(EXIT_SUCCESS);
}

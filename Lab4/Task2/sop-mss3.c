#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
    pthread_mutex_t *print_mutex;
    pthread_barrier_t *barrier;
    int *game_over;
    struct thread_arg_t *right_player;
    int received_card;
    sig_atomic_t *finish;
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

int seven_of_a_kind(int *deck){
    int suits[4] = {0};
    for(int i = 0; i < HAND_SIZE ;i++){
        switch (deck[i]%4)
        {
        case 0:
            /* code */
            suits[0]++;
            break;
        case 1:
            /* code */
            suits[1]++;
            break;
        case 2:
            /* code */
            suits[2]++;
            break;
        case 3:
            /* code */
            suits[3]++;
            break;
        
        }
    }
    for(int i = 0; i < 4 ;i++){
        if(suits[i] == 7){
            return 1;
        }
    }
    return 0;
}
void *thread_func(void *void_arg ){

    thread_arg_t *arg = (thread_arg_t*)void_arg;

    pthread_mutex_lock(arg->print_mutex);
    printf("Player %d\n",arg->id);
    print_deck(arg->hand,HAND_SIZE);
    printf("\n");
    pthread_mutex_unlock(arg->print_mutex);
    pthread_barrier_wait(arg->barrier);


    while(1){
        if(seven_of_a_kind(arg->hand) == 1){
            pthread_mutex_lock(arg->print_mutex);
            *(arg->game_over) = 1;
            printf("Player %d: My ship has sailed!\n",arg->id);
            print_deck(arg->hand,HAND_SIZE);
            pthread_mutex_unlock(arg->print_mutex);
            
        }
        pthread_barrier_wait(arg->barrier);
        if(*(arg->game_over) == 1) break;


        pthread_mutex_lock(arg->print_mutex);
        shuffle(arg->hand,HAND_SIZE);
        arg->right_player->received_card = arg->hand[0];
        pthread_mutex_unlock(arg->print_mutex);
        pthread_barrier_wait(arg->barrier);
        arg->hand[0] = arg->received_card;
        
        pthread_barrier_wait(arg->barrier);



    }

    return NULL;
}
int main(int argc, char *argv[])
{   
    printf("PID: %d\n",getpid());

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

    //Signal

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    sigaddset(&mask,SIGINT);

    if(pthread_sigmask(SIG_BLOCK,&mask,NULL) != 0) ERR("pthread_mask");

    //loop
    int current_players = 0;
    pthread_t tids[n];
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier,NULL,n);
    thread_arg_t args[n];
    pthread_mutex_t print_mutex =PTHREAD_MUTEX_INITIALIZER;
    int game_over = 0;
    while(1){
        int sig;

        if(sigwait(&mask,&sig) != 0)ERR("sigwait");
        
        if(sig == SIGUSR1){
            if(current_players < n)
            {
                for(int j = 0; j < HAND_SIZE; j++){
                    args[current_players].hand[j] = deck[j + (current_players * HAND_SIZE)];
                }
                args[current_players].barrier = &barrier;
                args[current_players].id = current_players;
                args[current_players].print_mutex = &print_mutex;
                args[current_players].game_over = &game_over;
                int right_idx = (current_players + 1) % n;
                args[current_players].right_player = &args[right_idx];
                int err = pthread_create(&tids[current_players],NULL,thread_func,&args[current_players]);
                if(err != 0){
                    errno = err;
                    ERR("pthread_create");
                }
                current_players++;

                if(current_players == n)
                {
                    for(int i = 0; i < n; i++)
                    {
                        pthread_join(tids[i],NULL);
                    }
                    puts("Game over. reseting table");

                    current_players = 0;
                    game_over = 0;
                    shuffle(deck,DECK_SIZE);
                }
                
            }else{
                printf("Table is full!.\n");
            }
        }
        if(sig == SIGINT)
        {
            if(current_players == n){
                game_over = 1;
                for(int i = 0; i < n; i++){
                    pthread_join(tids[i],NULL);
                }
            }else if(current_players > 0){
                for(int i = 0; i < current_players;i++){
                    pthread_cancel(tids[i]);
                    pthread_join(tids[i],NULL);
                }
            }
            puts("\nCaught SIGINT. celaning up...");
            break;
        }

    }
    

    pthread_barrier_destroy(&barrier);
    exit(EXIT_SUCCESS);
}

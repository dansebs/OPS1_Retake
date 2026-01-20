#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define DECK_SIZE (4 * 13)
#define HAND_SIZE (7)

// Table States
#define STATE_WAITING 0
#define STATE_PLAYING 1
#define STATE_EXIT   -1

void usage(const char *program_name) {
    fprintf(stderr, "USAGE: %s n\n", program_name);
    exit(EXIT_FAILURE);
}

typedef struct thread_arg_t {
    int hand[HAND_SIZE];
    int id;
    int received_card;
    
    // Game pointers
    pthread_mutex_t *print_mutex;
    pthread_barrier_t *barrier;
    int *game_over;
    struct thread_arg_t *right_player;
    
    // Condition Variable pointers (The "Waiting Room")
    pthread_mutex_t *start_mutex;
    pthread_cond_t  *start_cond;
    int *table_state;
    
} thread_arg_t;

void shuffle(int *array, size_t n) {
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void print_deck(const int *deck, int size) {
    const char *suits[] = {" of Hearts", " of Diamonds", " of Clubs", " of Spades"};
    const char *values[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};
    char buffer[1024];
    int offset = 0;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[");
    for (int i = 0; i < size; ++i) {
        int card = deck[i];
        int suit = card % 4;
        int value = card / 4;
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s%s", values[value], suits[suit]);
        if (i < size - 1) offset += snprintf(buffer + offset, sizeof(buffer) - offset, ", ");
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "]");
    puts(buffer);
}

int seven_of_a_kind(int *deck) {
    int suits[4] = {0};
    for (int i = 0; i < HAND_SIZE; i++) suits[deck[i] % 4]++;
    for (int i = 0; i < 4; i++) if (suits[i] == 7) return 1;
    return 0;
}

void *thread_func(void *void_arg) {
    thread_arg_t *arg = (thread_arg_t *)void_arg;

    // 1. Initial Print
    pthread_mutex_lock(arg->print_mutex);
    printf("Player %d sat down.\n", arg->id);
    print_deck(arg->hand, HAND_SIZE);
    pthread_mutex_unlock(arg->print_mutex);

    // 2. THE WAITING ROOM (Condition Variable Logic)
    pthread_mutex_lock(arg->start_mutex);
    while (*(arg->table_state) == STATE_WAITING) {
        // This releases the lock and waits. When signaled, it re-acquires the lock.
        pthread_cond_wait(arg->start_cond, arg->start_mutex);
    }
    
    // Check why we woke up
    int state = *(arg->table_state);
    pthread_mutex_unlock(arg->start_mutex);

    // If Host said "Get out" (SIGINT), we exit nicely.
    if (state == STATE_EXIT) {
        return NULL;
    }

    // Otherwise, STATE_PLAYING, so we proceed to the game.
    
    // 3. Game Synchronization (Barrier)
    // We still use the barrier to align the *start* of the rounds
    pthread_barrier_wait(arg->barrier);

    while (1) {
        if (seven_of_a_kind(arg->hand) == 1) {
            pthread_mutex_lock(arg->print_mutex);
            *(arg->game_over) = 1;
            printf("Player %d: My ship has sailed!\n", arg->id);
            print_deck(arg->hand, HAND_SIZE);
            pthread_mutex_unlock(arg->print_mutex);
        }
        
        pthread_barrier_wait(arg->barrier);
        if (*(arg->game_over) == 1) break;

        // Shuffle/Pass Logic
        pthread_mutex_lock(arg->print_mutex);
        shuffle(arg->hand, HAND_SIZE);
        arg->right_player->received_card = arg->hand[0];
        pthread_mutex_unlock(arg->print_mutex);

        pthread_barrier_wait(arg->barrier);
        arg->hand[0] = arg->received_card;
        pthread_barrier_wait(arg->barrier);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    printf("PID: %d\n", getpid());
    srand(time(NULL));

    if (argc != 2) usage(argv[0]);
    int n = atoi(argv[1]);
    if (n < 4 || n > 7) usage(argv[0]);

    int deck[DECK_SIZE];
    for (int i = 0; i < DECK_SIZE; ++i) deck[i] = i;
    shuffle(deck, DECK_SIZE);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) ERR("pthread_mask");

    // Resources
    pthread_t tids[n];
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, n);
    pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
    thread_arg_t args[n];

    // NEW: Condition Variable Resources
    pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
    int table_state = STATE_WAITING; // Shared state flag

    int current_players = 0;
    int game_over = 0;

    while (1) {
        int sig;
        if (sigwait(&mask, &sig) != 0) ERR("sigwait");

        if (sig == SIGUSR1) {
            if (current_players < n) {
                // Setup Args
                for (int j = 0; j < HAND_SIZE; j++) args[current_players].hand[j] = deck[j + (current_players * HAND_SIZE)];
                args[current_players].barrier = &barrier;
                args[current_players].id = current_players;
                args[current_players].print_mutex = &print_mutex;
                args[current_players].game_over = &game_over;
                args[current_players].received_card = 0;
                
                // Pass CV Resources
                args[current_players].start_mutex = &start_mutex;
                args[current_players].start_cond = &start_cond;
                args[current_players].table_state = &table_state;

                int right_idx = (current_players + 1) % n;
                args[current_players].right_player = &args[right_idx];

                if (pthread_create(&tids[current_players], NULL, thread_func, &args[current_players]) != 0) ERR("create");
                
                current_players++;

                if (current_players == n) {
                    // 1. Table Full -> Start Game
                    pthread_mutex_lock(&start_mutex);
                    table_state = STATE_PLAYING;       // Set flag
                    pthread_cond_broadcast(&start_cond); // Wake everyone up!
                    pthread_mutex_unlock(&start_mutex);

                    // 2. Wait for game to finish
                    for (int i = 0; i < n; i++) pthread_join(tids[i], NULL);
                    
                    puts("Game over. Resetting table.");
                    
                    // 3. Reset for next group
                    current_players = 0;
                    game_over = 0;
                    table_state = STATE_WAITING; // Reset state
                    shuffle(deck, DECK_SIZE);
                }
            } else {
                printf("Table is full!\n");
            }
        }
        
        if (sig == SIGINT) {
            puts("\nCaught SIGINT...");
            
            if (current_players == n) {
                // Scenario A: Game is running. Use game_over flag.
                game_over = 1;
                // Note: We might need to wake them if they were waiting at start,
                // but if n==current, they are already past the CV.
                for (int i = 0; i < n; i++) pthread_join(tids[i], NULL);
            } 
            else if (current_players > 0) {
                // Scenario B: Players are waiting in the "Lobby" (CV)
                // We don't need to cancel! We just tell them to leave.
                
                pthread_mutex_lock(&start_mutex);
                table_state = STATE_EXIT;          // Change state to "EXIT"
                pthread_cond_broadcast(&start_cond); // Wake them up
                pthread_mutex_unlock(&start_mutex);

                for (int i = 0; i < current_players; i++) pthread_join(tids[i], NULL);
            }
            
            puts("Cleaning up and exiting.");
            break;
        }
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&print_mutex);
    pthread_mutex_destroy(&start_mutex);
    pthread_cond_destroy(&start_cond);
    exit(EXIT_SUCCESS);
}
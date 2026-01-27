#include <semaphore.h>
#include <stdio.h>

sem_t sem_slots;

void example() {
    // 1. Init: (pointer, pshared: 0 for threads, initial_value)
    sem_init(&sem_slots, 0, 10); 

    // 2. Wait (Decrement): Blocks if value is 0
    sem_wait(&sem_slots); 

    // 3. Post (Increment): Wakes up a waiter
    sem_post(&sem_slots);

    // 4. Destroy
    sem_destroy(&sem_slots);
}
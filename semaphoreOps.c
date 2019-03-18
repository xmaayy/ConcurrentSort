#include <stdio.h>
#include "semun.h"
#include <sys/sem.h>

/*
 * Initialize a semaphore; this must be called before the semaphore can be
 * used.
 */
int set_semvalue(int sem_id, int val) {
    union semun sem_union;

    sem_union.val = val;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
        // semctl() failure
        return 0;
    } else {
        // method success
        return 1;
    }
}

/*
 * Remove a semaphore's ID.
 */
int del_semvalue(int sem_id) {
    union semun sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1) {
        // semctl() failure
        perror("Failed to delete semaphore");
        return 0;
    } else {
        // method success
        return 1;
    }
}

/*
 * Claim a semaphore, typically before entering a critical section.
 * If the semaphore is currently claimed, wait until it is free, then proceed.
 * Also known as P().
 */
int sem_claim(int sem_id) {
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = -1; // Operation -1 corresponds to P()
    sem_b.sem_flg = SEM_UNDO;   // SEM_UNDO allows the OS to release the 
                                // semaphore if the process exits abruptly.
    // Try to change the value of the semaphore.
    if (semop(sem_id, &sem_b, 1) == -1) {
        perror("sem_claim failed\n");
        return(0);
    }
    return 1;
}

/*
 * Release a semaphore, after executing a critical section.
 * Also known as V().
 */ 
int sem_release(int sem_id) {
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = 1; // Operation 1 corresponds to V()
    sem_b.sem_flg = SEM_UNDO;   // SEM_UNDO allows the OS to release the 
                                // semaphore if the process exits abruptly.
    // Try to change the value of the semaphore.
    if (semop(sem_id, &sem_b, 1) == -1) {
        perror("sem_release failed\n");
        return(0);
    }
    return 1;
}

/**
 * 
 */

int sem_check(int sem_id){
    return semctl(sem_id, 0, GETVAL);
}


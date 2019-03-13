/**
 *  Written on the 3rd of march, modeled after the solution to the 
 *  5 dining philosophers example 
 **/
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

//#include "sem.h"
#include "semaphoreOps.h"
#include <sys/sem.h>
#include "semun.h"

#define LISTSZ 5
#define NUMPROC 4

typedef struct number{
    int val;
    int sem_id;
} number_t;

typedef struct worker{
    pid_t pid; // The workers PID
    int place; // The worker's place at the table.
    number_t* nums[2]; // Pointers to the numbers its responsible for
} worker;


/**
 * We need to allocate shm for each number because I cant be bothered
 * with how C deals with pointers to arrays of structures and how
 * that interacts with my IDE's type checker
 **/
void init_shm(int *num_ids){
    for(int i = 0; i<LISTSZ;i++){
         num_ids[i] = shmget(1010+i, sizeof(number_t)*LISTSZ, 0666 | IPC_CREAT);
         // If the ID is -1 it failed
        if(num_ids[i] == -1) {
            // shmget() failed to create shared memory.
            perror("Failed to create shared memory.");
            exit(EXIT_FAILURE);
        }
    }       
    printf("Allocated shared memory.\n");
}

// Array containing pointers to each struct in shared memory
number_t* nums[LISTSZ];

// This method gets called with nums (above) as a parameter
void printArray(number_t** num) {
    for (int i=0; i<LISTSZ; i++) {
        printf("%d", num[i]->val); // compiles fine but doesn't work
    }
    puts(""); // newline
}

void init_array(int* mem_id) {
    int DEFAULTS[5] = {5,6,8,2,7};

    //Grab all locks and put in default values
    printf("Parent aquiring locks and filling values.\n");
    for(int i = 0; i<LISTSZ; i++){
        // Put the shared memory key into the array.
        nums[i] = (number_t*)shmat(mem_id[i], (void*)0, 0);

        // Assign a sempahore to the number.
        nums[i]->sem_id = semget((key_t)6666+i, 1, 0666|IPC_CREAT);
        //printf("Semaphore ID: %d\n", nums[i]->sem_id);

        // Set the semaphore to 1.
        set_semvalue(nums[i]->sem_id);

        // Decrement semaphore, blocking if it's already in use (it shouldn't be).
        sem_claim(nums[i]->sem_id);

        // Initialize the value of the number.
        nums[i]->val = DEFAULTS[i];
    } //At this point the parent holds all the locks
    printf("Parent holds all locks and values are initialized.\n");
    
    printf("The array is: ");
    printArray(nums);
}

void run_sort(int *mem_id) {    
    struct worker phil;
    pid_t pids[NUMPROC];
    int count;  // I dont use count in the for loop
                // because I used `i` first and dont wanna change
    for(int i = 0; i<NUMPROC; i++){
        pids[i] = fork();
        if(pids[i] == -1){
            // fork failed to create child. Terminate program.
            perror("Failed to create child process.\n");
            exit(EXIT_FAILURE);
        } else if (pids[i] == 0) {
            // Initialize this child process.
            // Let its assigned worker know its pid and the number_t-s for which
            // it is responsible.
            phil.pid = pids[i];
            phil.place = i;
            phil.nums[0]->val = i;
            phil.nums[1]->val = (i+1)%5;
            count = i;
            break;
        }
    }   // Now we have all the child processes running. Now we need
        // to model them
    printf("Worker %d\n", count);
    // Take the value of count at the time this was forked, mod 5.
    //switch (pids[count%5])
    switch(pids[count%5])
    {
        case 0:
            printf("I am worker %d and I handle numbers %d and %d\n",\
                        count, phil.nums[0]->val, phil.nums[1]->val);
            
            while(1) {
                
                sem_claim(phil.nums[0]->sem_id);
            }
            exit(EXIT_SUCCESS);
            break;

        default:;
            // The parent process should wait until all the children have
            // finished working.
            int status;
            pid_t pid;
			int stillRunning = NUMPROC;
			while(stillRunning > 0) {
				pid = wait(&status);
			    printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
			    stillRunning--;			
			}
            break;
    }

}



int main(){
    // One semaphore per number
    //struct semaphore sems[LISTSZ];
    //int sem_keys[5];
    int num_ids[5];

    init_shm(num_ids);
    init_array(num_ids);

    run_sort(num_ids);

    return 0;
}





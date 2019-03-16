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
#include <stdbool.h>

//#include "sem.h"
#include "semaphoreOps.h"
#include <sys/sem.h>
#include "semun.h" 

#define LISTSZ 5
#define NUMPROC 4

typedef struct number_t {
    int val;
    int sem_id;
} number_t;

typedef struct worker {
    pid_t pid; // The workers PID
    int place; // The worker's place at the table.
    number_t* nums[2]; // Pointers to the numbers its responsible for
} worker_t;


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

void printArray(number_t* num[]) {
    for (int i=0; i<LISTSZ; i++) {
        printf("%d ", num[i]->val);
    }
    puts(""); // newline
}

int get_mean(number_t* num[]) {
    int sum = 0;
    for (int i=0; i<LISTSZ; i++) {
        sum += num[i]->val;
    }
    // Mean = sum / number of terms.
    return sum / LISTSZ;
}

int get_min(number_t* num[]) {
    int minimum = num[0]->val;
    for (int i=0; i<LISTSZ; i++) {
        if (num[i]->val < minimum) {
            minimum = num[i]->val;
        }
    }
    return minimum;
}

int get_max(number_t* num[]) {
    int maximum = num[0]->val;
    for (int i=0; i<LISTSZ; i++) {
        if (num[i]->val > maximum) {
            maximum = num[i]->val;
        }
    }
    return maximum;
}

void print_stats(number_t* num[]) {
    int mean = get_mean(num);
    int min = get_min(num);
    int max = get_max(num);

    printf("The mean value of the array is: %d.\n"
            "The minimum value of the array is: %d.\n"
            "The maximum value of the array is: %d.\n",
            mean, min, max);
}


number_t* nums[LISTSZ];
void init_array(int* mem_id) {
    int DEFAULTS[5] = {5,6,8,2,7};
    // Array containing pointers to each struct in shared memory
    
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


void run_sort(int *mem_id, bool debug) {    
    pid_t pids[NUMPROC];
    worker_t phil;
    int count;  // I dont use count in the for loop
                // because I used `i` first and dont wanna change
    for(int i = 0; i<NUMPROC; i++){
        pids[i] = fork();
        
        if(pids[i] == -1){
            // fork failed to create child. Terminate program.
            perror("Failed to create child process.\n");
            exit(EXIT_FAILURE);
        } else if (pids[i] == 0) {
            puts("Child process");
            // Initialize this child process.
            // Let its assigned worker know its pid and the number_t-s for which
            // it is responsible.
            phil.pid = pids[i];
            // Let the worker know its place at the table.
            phil.place = i;
            // Give the worker pointers to the two numbers it is responsible
            // for.
            phil.nums[0] = nums[i];
            phil.nums[1] = nums[i+1];
            //phil.nums[0]->val = i;
            //phil.nums[1]->val = (i+1)%5;
            count = i;
            printf("Created child process responsible for numbers %d and %d\n"
            ,phil.nums[0]->val, phil.nums[1]->val);
            break;
        }
    }   // Now we have all the child processes running. Now we need
        // to model them
    // Take the value of count at the time this was forked, mod 5.
    switch(pids[count%5])
    {
        case 0:
            printf("I am worker %d and I handle numbers %d and %d\n",\
                        count, phil.nums[0]->val, phil.nums[1]->val);
            
            exit(EXIT_SUCCESS);
            break;

        default:;
            puts("Default case");
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

// Function used at startup to ask the user whether they wish to use debug mode.
// Returns true if the user answers in the affirmative.
bool prompt_debug() {
    char answer[20];
    bool result = false;

    // Prompt the user.
    printf("Would you like to use debug mode? [y/n] ");
    scanf("%s", answer);
    // Only check the first character; this allows for "y", "yes", "Y", "Yes"...
    if (answer[0] == 'y' || answer[0] == 'Y') {
        puts("Running in DEBUG MODE");
        result = true;
    }
    return result;
} 



int main(int argc, char* argv[]){
    bool debug = false;
    debug = prompt_debug();

    /*
    // Program can be run with '-d' or '-v' (verbose) flag for debug mode.
    // NB: The assignment asks us to use the user-prompt method so this is
    // dummied out, but I think it's just nice to have; I have a bash script to
    // make and run the program so being able to just have a CLA saves a little
    // time.

    for(int i=0; i<argc; i++){
        if(argv[i][0] =='-' && (argv[i][1] == 'd' || argv[i][1] == 'v')() {
            puts("Running in DEBUG MODE");
            debug = 1;
        }
    }
    */


    // One semaphore per number
    //struct semaphore sems[LISTSZ];
    //int sem_keys[5];
    int num_ids[5];

    init_shm(num_ids);
    init_array(num_ids);
    

    run_sort(num_ids, debug);

    return 0;
}





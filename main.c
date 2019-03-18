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

// Semaphore-related includes.
#include <sys/sem.h>
#include "semun.h"

#include "semaphoreOps.h"
#include "sampleArrays.h"


// The number of items in the list.
#define LISTSZ 5
// The number of processes to use. Should be equal to LISTSZ-1.
//#define NUMPROC 4
#define NUMPROC LISTSZ-1

// Arbitrary value to start with for the numbering of shared memory keys.
#define STARTING_SEMID 1010
#define SORT_SEMID 1008

typedef struct number_t {
    int val;
    int sem_id;
} number_t;

typedef struct worker {
    pid_t pid; // The workers PID
    int place; // The worker's place at the table.
    number_t* nums[2]; // Pointers to the numbers it's responsible for.
} worker_t;


bool debug = false;

/**
 * This is used to keep track of the number of children currently priocessing
 * the array. It starts at 0, each child adds to it as they become active
 * and the final check for wether or not sorting is complete involves checking 
 * if children is NUMPROC
 **/
int children;

/* Function: init_shm
 * --------------------
 *  Initialize LISTSZ shared memory segments, each containing a number_t,
 *  and populate an array with the identifiers of these segments.
 *
 *  num_ids: Array of ints in which the SHM identifiers will be stored.
 */
void init_shm(int *num_ids){
    for(int i = 0; i<LISTSZ;i++){
         num_ids[i] = shmget(STARTING_SEMID+i, sizeof(number_t)*LISTSZ, 0666 | IPC_CREAT);
         // If the ID is -1 it failed
        if(num_ids[i] == -1) {
            // shmget() failed to create shared memory.
            perror("Failed to create shared memory.");
            exit(EXIT_FAILURE);
        }
    }       
    printf("Allocated shared memory.\n");
}

/* Function: print_Array
 * --------------------
 *  Print out the contents of the array of numbers.
 *
 *  num: The array of numbers to print out.
 */
void printArray(number_t* num[]) {
    for (int i=0; i<LISTSZ; i++) {
        printf("%d ", num[i]->val);
    }
    puts(""); // newline
}

/* Function: get_mean
 * --------------------
 *  Compute the mean of the array in shared memory.
 *
 *  num: The array of numbers.
 */
int get_mean(number_t* num[]) {
    int sum = 0;
    for (int i=0; i<LISTSZ; i++) {
        sum += num[i]->val;
    }
    // Mean = sum / number of terms.
    return sum / LISTSZ;
}

/* Function: get_max
 * --------------------
 *  Determine the maximum number contained in the array in shared memory.
 *
 *  num: The array of numbers.
 */
int get_min(number_t* num[]) {
    int minimum = num[0]->val;
    for (int i=0; i<LISTSZ; i++) {
        if (num[i]->val < minimum) {
            minimum = num[i]->val;
        }
    }
    return minimum;
}

/* Function: get_mean
 * --------------------
 *  Determine the minimum number contained in the array in shared memory.
 *
 *  num: The array of numbers.
 */
int get_max(number_t* num[]) {
    int maximum = num[0]->val;
    for (int i=0; i<LISTSZ; i++) {
        if (num[i]->val > maximum) {
            maximum = num[i]->val;
        }
    }
    return maximum;
}


/* Function: is_sorted
 * ---------------------
 * Returns the number of unsorted elements in the array
 * 0 if it is sorted in non-ascending order
 */
int is_sorted(number_t* num[]){
    int unsorted = 0;
    for(int i=0; i<LISTSZ-1; i++){
        if(num[i]->val < num[i+1]->val){
            return 1;
        }
    }
    return 0;
}


/* Function: print_stats
 * --------------------
 * Print out the mean, minimum, and maximum values of the numbers contained
 * in the array in shared memory.
 *
 *  num: The array of numbers.
 */
void print_stats(number_t* num[]) {
    int mean = get_mean(num);
    int min = get_min(num);
    int max = get_max(num);

    printf("The final sorted array is: ");
    printArray(num);
    printf("The mean value of the array is: %d.\n"
            "The minimum value of the array is: %d.\n"
            "The maximum value of the array is: %d.\n",
            mean, min, max);
}

// Array containing pointers to each number_t in use.
number_t* nums[LISTSZ];

/* Function: init_array
 * --------------------
 *  Get locks on each of the numbers in shared memory and populate their values.
 *
 *  mem_id: An array containing keys of all the shared memory segments.
 */
void init_array(int* mem_id, int* value_array) {
    //int DEFAULTS[5] = {5,6,8,2,7};
    // Array containing pointers to each struct in shared memory

    // Create the children semaphore and initialize it to 0
    children = semget((key_t)SORT_SEMID, 1, 0666|IPC_CREAT);
    set_semvalue(children, 0);

    //Grab all locks and put in default values
    printf("Parent aquiring locks and filling values.\n");
    for(int i = 0; i<LISTSZ; i++){
        // Put the shared memory key into the array.
        nums[i] = (number_t*)shmat(mem_id[i], (void*)0, 0);

        // Assign a sempahore to the number.
        nums[i]->sem_id = semget((key_t)6666+i, 1, 0666|IPC_CREAT);
        //printf("Semaphore ID: %d\n", nums[i]->sem_id);

        // Set the semaphore to 1.
        set_semvalue(nums[i]->sem_id, 1);

        // Decrement semaphore, blocking if it's already in use (it shouldn't be).
        //printf("Value of semaphore before claiming - %d\n", sem_check(nums[i]->sem_id));
        sem_claim(nums[i]->sem_id);
        //printf("Value of semaphore after claiming - %d\n", sem_check(nums[i]->sem_id));

        // Initialize the value of the number.
        nums[i]->val = value_array[i];
    } //At this point the parent holds all the locks
    printf("Parent holds all locks and values are initialized.\n");

    printf("Parent released locks. The array is: ");
    printArray(nums);
    
}

/**
 * Called when the children are notified that the sorting process is complete.
 * No input no output, just a straight up call to exit and thats itS
 */
void handle_done(int sig){
    printf("Parent notified child with PID %d that execution is complete\n", getpid());
    exit(EXIT_SUCCESS);
}

/**
 * We need to make sure that we never get only one lock. 
 * If we do then the system can deadlock. (we should probably
 * use a second sem for getting sems but w/e)
 */
int get_locks(worker_t *worker){
    while(true){
        if(sem_check(worker->nums[0]->sem_id) > 0 && 
            sem_check(worker->nums[1]->sem_id) > 0)
        {

            sem_claim(worker->nums[0]->sem_id);
            sem_claim(worker->nums[1]->sem_id);
            break;
        } 
    }
    return 0;
}


/* Function: run_sort
 * --------------------
 *  Execute the sorting. See the README for the algorithm used. If debug mode is
 *  active, the child processes should print out whether or not they swap the
 *  two numbers they are responsible for.
 *
 *  mem_id: An array containing keys of all the shared memory segments.
 *  debug:  A boolean indicating whether the user has requested extra debugging
 *          logging.
 */
void run_sort(int *mem_id) {    
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
            count = i;
            printf("Created child process responsible for numbers %d and %d\n"
            ,phil.nums[0]->val, phil.nums[1]->val);
            break;
        }
    }   // Now we have all the child processes running. Now we need
        // to model them
    switch(pids[count%4])
    {
        case 0:;
            // Register signal handler for when parent determines the array
            // is fully sorted
            int temp;

            // This release essentially shows the main loop that this is active
            sem_release(children);

            signal(SIGUSR1, handle_done);
            printf("I am worker %d and I handle numbers %d and %d\n",\
                        count, phil.nums[0]->val, phil.nums[1]->val);
            usleep(10000);  // Otherwise the children start sorting before the others
                            // have started. Its still correct but would look odd
            // Keep executing this loop until we're told to stop
            // Basically, if a switch is needed, get locks, switch and
            // release the lock
            while(true){
                if(phil.nums[0]->val < phil.nums[1]->val){
                    sem_claim(children); // There should NEVER be a block here because
                                        // this child added one to this sem
                    get_locks(&phil);

                    // Do the switch
                    if(debug){
                        printf("Child %d switching %d with %d\n", 
                        phil.place, phil.nums[0]->val, phil.nums[1]->val);
                    }
                    temp = phil.nums[0]->val;
                    phil.nums[0]->val = phil.nums[1]->val;
                    phil.nums[1]->val = temp;

                    // Release all semaphores
                    sem_release(phil.nums[0]->sem_id);
                    sem_release(phil.nums[1]->sem_id);
                    sem_release(children);
                } else {
                    if(debug) {
                        printf("Child %d not switching %d with %d\n", 
                            phil.place, phil.nums[0]->val, phil.nums[1]->val);
                    }
                }
            }
            break;

        default:;
            // The parent process should wait until all the children have
            // finished working.
            int status;
            pid_t pid;

            // Release locks so child processes can start as soon as they're created
            // this could also be done in the parent process once all the children have 
            // been created. It ends up being the same order of switching but impoerceptably
            // faster if the children dont have to wait after being created
            for(int i = 0; i<LISTSZ; i++){
                sem_release(nums[i]->sem_id);
            }
            printf("Children are ready\n");
            // If its unsorted or there is a locked process we cant consider
            // the sorting job finished
			while(sem_check(children)<NUMPROC | is_sorted(nums)) {}

            // We now know its sorted because the above loop completed. 
            // We need to notify all the children that the process is complete
            for(int i = 0; i<NUMPROC; i++){
                kill(pids[i], SIGUSR1);
            }

            //The proof is in the final state of the array
            sleep(0);
            print_stats(nums);

            break;
    }

}


/* Function: get_nums
 * --------------------
 *  Function used at startup to ask the user what numbers should
 *  be used for the stats/sorting portion of the code
 */
void get_nums(int nums[LISTSZ]){
    printf("Please input 5 numbers:\n");
    int number;
    for(int i=0; i<LISTSZ; i++){
        printf(">");
        scanf("%d", &number);
        nums[i] = number;
    }

}


/* Function: prompt_debug
 * --------------------
 *  Function used at startup to ask the user whether they wish to use debug mode.
 *  Returns true if the user answers in the affirmative.
 */
void prompt_debug() {
    char answer[20];

    // Prompt the user.
    printf("Would you like to use debug mode? [y/n] ");
    scanf("%s", answer);
    // Only check the first character; this allows for "y", "yes", "Y", "Yes"...
    if (answer[0] == 'y' || answer[0] == 'Y') {
        puts("Running in DEBUG MODE");
        debug = true;
    }
} 


int main(int argc, char* argv[]){
    prompt_debug();

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

    // Array to hold shared memory keys.
    int num_ids[LISTSZ];
    // Set up the shared memory.
    init_shm(num_ids);

   // If a number was supplied as an argument, pick a test case corresponding
   // to it.
    int testCase = 0;
    int array[LISTSZ];
    if (argc > 1) {
        int arg = argv[1][0] - '0';
        if (arg >= 0 && arg <= NUMBER_OF_TESTS) {
            printf("Using test case number %d.\n", arg);
            testCase = arg;
        } else {
            puts("Invalid test number specified; using default test case(0).");
        }
        // Populate the shared memory with values.
        init_array(num_ids, test_arrays[testCase]);
    } else {
        // Populate the shared memory with values.
        get_nums(array);
        init_array(num_ids, array);
    }
    // Run the sorting.
    run_sort(num_ids);

    return 0;
}





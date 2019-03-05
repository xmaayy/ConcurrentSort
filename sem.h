#ifndef SEM_H
#define SEM_H

/**
* Use this for the proces wait queues. Stores the next element
* and the pid of a single process
**/
typedef struct process{
    pid_t pid;
    struct process *next;
}process;

struct process * makeProc(pid_t pid);
void freeProc(struct process *proc);


/**
* This struct represents a single semaphore. The wait list should
* function as a FIFO queue, so we keep it double ended to maintain
* constant time push/pop
**/
typedef struct semaphore{
    int value;
    struct process *list_start; // Double ended so we dont have to
    struct process *list_end;   // scale inserts linearly
}semaphore;

void semWait(struct semaphore *sem);

void semSig(struct semaphore *sem);

pid_t getWaiting(struct semaphore *sem);

int addProcess(struct semaphore *sem, pid_t pid);


#endif
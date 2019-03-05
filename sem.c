#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "sem.h"

/**
 *  This function handles the allocation of new waiting processes on
 *  the heap, allowing us to maintain a queue of waiting processes
 *  for each semaphore (overkill but necessary) 
 **/
struct process * makeProc(pid_t pid){
    // Allocate the waiting process on the heap
    struct process * proc_ptr;
    proc_ptr = (struct process *)malloc(sizeof(struct process));

    //Set its pid
    proc_ptr->pid = pid;
    proc_ptr->next = NULL;

    return proc_ptr;
}

/**
 *  Add a process to the waiting queue. Uses heap allocated mem
 *  to keep the list for each semaphore. Returns 1 if successful
 **/
int addProcess(struct semaphore *sem, pid_t pid){
    if(sem->list_start == NULL){
        sem->list_start = makeProc(pid);
        sem->list_end = sem->list_start;
    } else {
        sem->list_end->next = makeProc(pid);
    }
    return 1;
}


/**
 *  Gets the first waiting process. Schedules naively via FIFO and
 *  pays no attention to any kind of priority
 **/
pid_t getWaiting(struct semaphore *sem){
    struct process *proc;

    // Grab the first element and move the list pointer forward in
    // the list
    // TODO: Add checking to make sure we dont do any nullrefs
    proc = sem->list_start;
    sem->list_start = proc->next;

    // grab pid, free the mem and return
    pid_t proc_id = proc->pid;
    free(proc);
    return proc_id;
}

/**
 *  Decrements semaphore and blocks itsself if the resource was
 * already in use
 **/
void semWait(struct semaphore *sem){
    sem->value--;
    if(sem->value < 0){ // If it was already busy
        addProcess(sem, getpid());
        pause();
    }
    printf("Process %d now holds a new lock\n", getpid());
}

void semSig(struct semaphore *sem){
    sem->value++;
    if(sem->value <= 0){
        pid_t waiting_proc = getWaiting(sem);
        kill(waiting_proc, SIGUSR1);
    }
}
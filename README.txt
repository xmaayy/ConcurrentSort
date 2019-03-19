To run the program, first run 'make' to update the binary (the one already
compiled should work, but no sense in taking risks). Then, run the binary "./STATS".
By default using make, the binary will be called STATS as is specified in the document. 

You can optionally specify a number as a command-line argument. This number 
selects one of the base test cases that will be used; all the test cases can be found in
sampleArrays.h, and these test cases do include the two mandatory cases 
specified in the assignment description.

If no number is specified, the program will instead prompt the user for five
integers, which it will use to populate the array.

Description of files:
main.c contains the core of the program.
semaphoreOps.c contains all the methods related to semaphores.
sampleArrays.h contains all of the test cases.

/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Shell Simulator 

This program will take command line arguments to spawn that many processes to start, and
keep only that many alive at once. Oss makes a simulated clock which increments every loop.
The processes forked off by main then loop over a critical section, checking to see if they are supposed to terminate yet. If they are, they leave a message in shared memory which oss
writes to a file.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "semnam.h"

int main(int argc, char *argv[]){
	
	pid_t pid = (long)getpid();		// process id
	
	/* SEMAPHORE INFO */
	sem_t *semaphore = sem_open(SEM_NAME, O_RDWR);
	if (semaphore == SEM_FAILED) {
		printf("%ld: ", pid);
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }
	/* SEMAPHORE INFO END */
	
	
	/* VARIOUS VARIABLES */
	srand(time(NULL));
	int wait, begin, end;			// how long to wait
									// when the process began
									// when to end
	int stop = 1;					// when to end loop
					

	/* SHARED MEMORY */
	int shmidA, shmidB;
	key_t keyA, keyB;
	keyA = 1001;
	keyB = 888;
	
	// locate the segment
	if ((shmidA = shmget(keyA, 50, 0666)) < 0) {
        perror("child shmget failed");
        exit(1);
    }
	else if ((shmidB = shmget(keyB, 50, 0666)) < 0) {
        perror("child shmget failed");
        exit(1);
    }
	
	// attach to our data space
	int *clock;
	int *shmMsg;
	if ((clock = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("child shmat failed");
        exit(1);
    }
	else if ((shmMsg = shmat(shmidB, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
        exit(1);
    }
	/* SHARED MEMORY END*/
	
	
	// generate random wait time
	begin = (clock[0] * 1000000000) + clock[1];
	wait = (rand() % 999999) + 1;
	end = begin + wait;
	
	while(stop == 1) {
		// wait on the semaphore
		if(sem_trywait(semaphore) != 0){
			continue;
		}
		
		/* CRITICAL SECTION */
		// if its end time has arrived check if shmMsg is empty entirely
		if(((clock[0] * 1000000000) + clock[1]) <= end){
			if(shmMsg[0] != 0 || shmMsg[1] != 0 || shmMsg[2] != 0){
				// shmMsg is full, can't do anything
			}
			else {
				// set the loop to end and post message to shmMsg
				stop = 0;
				shmMsg[0] = pid;
				shmMsg[1] = clock[0];
				shmMsg[2] = clock[1];
			}
			
		}
		/* CRITICAL SECTION END */
		
		// let go of the semaphore
		sem_post(semaphore);
	}
	
	return 0;
}
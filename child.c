/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Shell Simulator 

In this project I created an empty shell of an OS simulator and do some very basic tasks in preparation for a more comprehensive simulation later. It uses fork, exec, shared memory and semaphores.

This project (as far as I can tell) meets all the requirements of the project specifications. However, it should be noted that the forking off of children by oss in the loop, often fails around ~20+ children. I am not entirely sure why this is. 
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
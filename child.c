/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Shell Simulator 
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

#define SEM_NAME "/te"

int main(int argc, char *argv[]){
	
	sem_t *semaphore = sem_open(SEM_NAME, O_RDWR);
	if (semaphore == SEM_FAILED) {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }
	
	pid_t pid = (long)getpid();			// process id
	
	srand(time(NULL));
	int wait, begin, end;
	int stop, try;
	stop = 1;		// when to end loop

	// shared memory
	int shmid;
	key_t key = 1001;
	
	// locate the segment
	if ((shmid = shmget(key, 50, 0666)) < 0) {
        perror("child shmget failed");
        exit(1);
    }
	
	// attach to our data space
	int *clock;
	if ((clock = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("child shmat failed");
        exit(1);
    }
	
	int *shmMsg;
	shmMsg = clock;
	
	// generate random wait time
	begin = (clock[0] * 1000000000) + clock[1];
	printf("%ld: Beginning at: %i nanoseconds.\n", pid, begin);
	wait = (rand() % 999999) + 1;
	end = begin + wait;
	
	do {
		// wait on the semaphore
		if (sem_wait(semaphore) < 0) {
				perror("sem_wait(3) failed on child");
				exit(1);
			}
			
			
		/* CRITICAL SECTION */
		// if its end time has arrived check if shmMsg is empty entirely
		if(((clock[0] * 1000000000) + clock[1]) <= end){
			// if it isnt, break and try again next time
			if(shmMsg[2] != 0 && shmMsg[3] != 0 && shmMsg[4] != 0){
				//printf("%ld: shmMsg is full!\n", pid);
			}
			else {
				printf("%ld: Time to end!\n", pid);
				stop = 0;
				shmMsg[2] = pid;
				shmMsg[3] = clock[0];
				shmMsg[4] = clock[1];
			}
		}
		
		// let go of the semaphore
		if (sem_post(semaphore) < 0) {
            perror("sem_post(3) error on child");
			exit(1);
        }
		
	} while(stop == 1);
	
	printf("%ld: FINISHED at: %i.%i.\n", pid, clock[0], clock[1]);
	
	return 0;
}
/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Simulator 
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>


int main(int argc, char *argv[]){
	
	pid_t pid = (long)getpid();			// process id
	
	srand(time(NULL));
	int wait, begin, end;

	
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
	
	// generate random wait time
	begin = (clock[0] * 1000000000) + clock[1];
	printf("%ld: Beginning at: %i nanoseconds.\n", pid, begin);
	wait = (rand() % 999999) + 1;
	printf("%ld: Waiting for: %i nanoseconds.\n", pid, wait);
	end = begin + wait;
	printf("%ld: Ending at: %i nanoseconds.\n", pid, end);
	
	while (((clock[0] * 1000000000) + clock[1]) < end){
		// loop constantly and wait until time
	}
	
	printf("%ld: FINISHED at: %i nanoseconds.\n", pid, end);
	
	return 0;
}
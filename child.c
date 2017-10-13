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
	//printf("I'm process %ld.\n", pid);
	
	// generating random #
	int r;
	srand(time(NULL));
	r = rand() % 5;
	
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
	
	printf("%ld: Simulated time is: %i seconds, %i nanoseconds.\n", pid, clock[0], clock[1]);
	printf("%ld: Sleeping for %i.\n", pid, r);
	sleep(r);
	printf("%ld: Simulated time is: %i seconds, %i nanoseconds.\n", pid, clock[0], clock[1]);
	
	return 0;
}
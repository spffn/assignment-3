/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Shell Simulator 
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <signal.h>
#include "semnam.h"

char errstr[50];

int main(int argc, char *argv[]){
	
	/* SEMAPHORE INFO */
	// init semaphore
	sem_t *semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, SEM_PERMS, MUTEX);
	
	if (semaphore == SEM_FAILED) {
		printf("Master: ");
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }
	
	// close it because we dont use the semaphore in the parent and we want it to
	// autodie once all processes using it have ended
	if (sem_close(semaphore) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
	/* SEMAPHORE INFO END */
	
	int spawnNum = 5;				// the # processes to spawn and limit on 
									// active processes at one time
	int activeKid = spawnNum;		// how many kids are active now?
	int kidLim = spawnNum;			// how many have been made in total
	
	// 1,000,000,000 ns = 1 seconds
	int clockInc = 50;					// increment simClock 500ns by default
	int simTimeEnd = 2;					// when to end the simulation
	pid_t pid;
	int status;
	
	// shared memory
	int shmid;
	key_t key;
	
	// file name
	char fname[] = "log.out";
	
	// the timer information
    time_t endwait;
    time_t start;
    int timeToWait = 20; 		// end loop after this time has elapsed
	
	// for printing errors
	snprintf(errstr, sizeof errstr, "%s: Error: ", argv[0]);
	int i, c;
	opterr = 0;	

	// parse the command line arguments
	while ((c = getopt(argc, argv, ":t:hl:s:i:o:")) != -1) {
		switch (c) {
			// set the clock increment in nanoseconds
			case 'i': {
				clockInc = atoi(optarg);
				printf ("Setting clock increment to: %i nanoseconds\n", spawnNum);
				break;
			}
			// setting name of file of palindromes to read from	
			// by default it is palindromes.txt
			case 'l': {
				int result;
				char exten[] = ".out";
				char newname[200];
				strcpy(newname, optarg);
				strcat(newname, exten);
				result = rename(fname, newname);
				if(result == 0) {
					printf ("File renamed from %s to %s\n", fname, newname);
					strcpy(fname, newname);
				}
				else {
					perror(errstr);
					printf("Error renaming file.\n");
				}
				break;
			}
			// set the amount of simulated time to run for
			case 'o': {
				simTimeEnd = atoi(optarg);
				printf ("Setting simulated clock end time to: %i nanoseconds\n", simTimeEnd);
				break;
			}
			// set the number of slave processes to spawn
			case 's': {
				spawnNum = atoi(optarg);
				activeKid = spawnNum;
				kidLim = spawnNum;
				printf ("Number of chidren to spawn set to: %i\n", spawnNum);
				break;
			}
			// sets the amount of time to let a program run until termination
			case 't': {
				timeToWait = atoi(optarg);
				printf ("Program run time set to: %i seconds\n", timeToWait);
				break;
			}
			// show help
			case 'h': {
				printf("\n----------\n");
				printf("HELP LIST: \n");
				printf("-i: \n");
				printf("\t Sets the simulation clock increment in nanoseconds.\n");
				printf("\t Default is 1,000 nanoseconds.\n");
				printf("\tex: -i 14000000 \n");
				
				printf("-h: \n");
				printf("\tShow help, valid options and required arguments. \n");
				
				printf("-l: \n");
				printf("\t Sets the name of the log file to output to.\n");
				printf("\t Default is log.out.\n");
				printf("\tex: -l filename \n");
				
				printf("-o: \n");
				printf("\t Sets the simulated time to end at.\n");
				printf("\t Default is 2 simulated seconds.\n");
				printf("\tex: -o 10 \n");
				
				printf("-s: \n");
				printf("\t Sets the maximum number of processes to spawn.\n");
				printf("\t Default is 5.\n");
				printf("\tex: -s 14 \n");
				
				printf("-t: \n");
				printf("\tSets the amount of real time seconds to wait before terminating program. \n");
				printf("\tDefault is 20 seconds. Must be a number.\n");
				printf("\tex: -t 60 \n");
				
				printf("----------\n\n");
				break;
			}
			// if no argument is given, print an error and end.
			case ':': {
				perror(errstr);
				fprintf(stderr, "-%s requires an argument. \n", optopt);
				return 1;
			}
			// if an invalid option is caught, print that it is invalid and end
			case '?': {
				perror(errstr);
				fprintf(stderr, "Invalid option(s): %s \n", optopt);
				return 1;
			}
		}
	}
	
	/* SHARED MEMORY */
	key = 1001;
	// shared memory clock
	// [0] is seconds, [1] is nanoseconds
	int *clock;
	int *shmMsg;
	
	// create segment to hold all the info from file
	if ((shmid = shmget(key, 50, IPC_CREAT | 0666)) < 0) {
        perror("Master shmget failed.");
        exit(1);
    }
	
	// attach segment to data space
	if ((clock = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
        exit(1);
    }
	
	// connect shmMsg to shared memory
	// clock[2], [3] and [4] will technically be shmMsg
	// 2 is the child pi
	// 3 and 4 is the time
	shmMsg = clock;
		
	// write to shared memory the intial clock settings
	// clear out shmMsg
	clock[0] = 0;
	clock[1] = 0;
	shmMsg[2] = 0;
	shmMsg[3] = 0;
	shmMsg[4] = 0;
	/* SHARED MEMORY STUFF END*/

	// start forking off processes
	pid_t (*cpids)[spawnNum] = malloc(sizeof *cpids);
	for(i = 0; i < spawnNum; i++){
		(*cpids)[i] = fork();
		if ((*cpids)[i] < 0) {
			perror(errstr); 
			printf("Fork failed!\n");
			exit(1);
		}
		else if ((*cpids)[i] == 0){
			// pass to the execlp the name of the code to exec
			// increase the # of currently activeKids
			execlp("child", "child", NULL);
			perror(errstr); 
			printf("execl() failed!\n");
			exit(1);
		}
	}
	
	// open file for writing to
	FILE *f = fopen(fname, "w");
	if(f == NULL){
		perror(errstr);
		printf("Error opening file.\n");
		exit(1);
	}
		
	// calculate end time
	start = time(NULL);
	endwait = start + timeToWait;
	printf("Master: Starting clock loop at %s.\n", ctime(&start));
	
	// master waits until the real or simulated time limit runs out or 100 
	// kids have been spawned in total
    while (clock[0] < simTimeEnd && start < endwait && kidLim < 100) {  
		int who;
		
		// increment the clock
		start = time(NULL);
		clock[1] += clockInc;
		if(clock[1] - 1000000000 > 0){
			clock[1] -= 1000000000; 
			clock[0] += 1;
		}
		
		// check to see if shmMsg is empty entirely
		// if not, write to file using the information in it and then clear it
		// else just keep going
		if(shmMsg[2] != 0 || shmMsg[3] != 0 || shmMsg[4] != 0){
			for(i = 0; i < spawnNum; i++){
				if((*cpids)[i] == shmMsg[2]){
					who = i;
				}
			}
			printf("Master: Child %d is terminating at Master time %d.%d | Message recieved at %d.%d\n", shmMsg[2], clock[0], clock[1], shmMsg[3], shmMsg[4]);
			fprintf(f, "Master: Child %d is terminating at Master time %d.%d | Message recieved at %d.%d\n", shmMsg[2], clock[0], clock[1], shmMsg[3], shmMsg[4]);
			shmMsg[2] = 0;
			shmMsg[3] = 0;
			shmMsg[4] = 0;
			activeKid--;					// remove an active kid
			printf("Master: %i active kids.\n", activeKid);
			
			// if we do this, it means one of the kids has terminated, so we 
			// need to make another and overwrite the previous entry in the
			// process list with this new one
			(*cpids)[who] = fork();
			if ((*cpids)[who] < 0) {
				perror(errstr); 
				printf("Fork failed!\n");
				continue;
			}
			else if ((*cpids)[who] == 0){
			// pass to the execlp the name of the code to exec
				execlp("child", "child", NULL);
				perror(errstr); 
				printf("execl() failed!\n");
				exit(1);
			}
			
			activeKid++;	// increase active kids
			kidLim++;		// increase child limit
			
			printf("Master: New child %ld spawned.\n", (*cpids)[who]);
			printf("Master: %i total kids spawned.\n", kidLim);
		}
		
	}
	
	start = time(NULL);
	printf("Master: Ending clock loop at %s", ctime(&start));
	printf("Master: Simulated time ending at: %i seconds, %i nanoseconds.\n", clock[0], clock[1]);
	
	if(activeKid > 0){
		printf("Master: %ld kids remaining.\n", activeKid);
		for(i = 0; i < spawnNum; i++){
			if(kill((*cpids)[i], 0)){
				printf("Master: Killing %ld.\n", (*cpids)[i]);
				kill((*cpids)[i], SIGTERM);
			}
		}
	}
	
	/* CLEAN UP */
	shmctl(shmid, IPC_RMID, NULL);
	
	fclose(f);
	
	if (sem_unlink(SEM_NAME) < 0){
        perror("sem_unlink(3) failed");
		exit(1);
	}
	
	free(cpids);
	
	sem_close(semaphore);

    return 0;
}
/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Shell Simulator 

In this project I created an empty shell of an OS simulator and do some very basic tasks in preparation for a more comprehensive simulation later. It uses fork, exec, shared memory and semaphores.

This project (as far as I can tell) meets all the requirements of the project specifications. However, it should be noted that the forking off of children by oss in the loop, often fails around ~20+ children. I am not entirely sure why this is. 
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
sem_t *semaphore;
FILE *f;
int shmidA, shmidB;
pid_t *pcpids;

void sig_handler(int);

int main(int argc, char *argv[]){
	
	signal(SIGINT, sig_handler);
	
	/* SEMAPHORE INFO */
	// init semaphore
	semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, SEM_PERMS, MUTEX);
	
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
	
	
	/* VARIOUS VARIABLES */
	int spawnNum = 5;				// the # processes to spawn and limit on 
									// active processes at one time
	int activeKid = spawnNum;		// how many kids are active now?
	int kidLim = spawnNum;			// how many have been made in total
	
	// 1,000,000,000 ns = 1 seconds
	int clockInc = 50;				// increment simClock 50 ns by default
	int simTimeEnd = 2;				// when to end the simulation
	pid_t pid;
	int status;
	
	// file name
	char fname[] = "log.out";
	
	// the timer information
    time_t endwait;
    time_t start;
    int timeToWait = 20; 			// end loop after this time has elapsed
	
	// for printing errors
	snprintf(errstr, sizeof errstr, "%s: Error: ", argv[0]);
	int i, c;
	opterr = 0;	

	/* COMMAND LINE PARSING */
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
			// max is 19
			case 's': {
				spawnNum = atoi(optarg);
				if(spawnNum > 19){
					printf("Can only set a max of 19 processes to run at one time.\n");
					spawnNum = 19;
				}
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
	/* COMMAND LINE PARSING END */
	
	
	/* SHARED MEMORY */
	key_t keyA, keyB;
	keyA = 1001;
	keyB = 888;
	// shared memory clock
	// [0] is seconds, [1] is nanoseconds
	int *clock;
	int *shmMsg;
	
	// create segment to hold all the info from file
	if ((shmidA = shmget(keyA, 50, IPC_CREAT | 0666)) < 0) {
        perror("Master shmget failed.");
        exit(1);
    }
	else if ((shmidB = shmget(keyB, 50, IPC_CREAT | 0666)) < 0) {
        perror("Master shmget failed.");
        exit(1);
    }
	
	// attach segment to data space
	if ((clock = shmat(shmidA, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
        exit(1);
    }
	else if ((shmMsg = shmat(shmidB, NULL, 0)) == (char *) -1) {
        perror("Master shmat failed.");
        exit(1);
    }
		
	// write to shared memory the intial clock settings
	// clear out shmMsg
	clock[0] = 0;
	clock[1] = 0;
	shmMsg[0] = 0;
	shmMsg[1] = 0;
	shmMsg[2] = 0;
	/* SHARED MEMORY END*/

	
	// open file for writing to
	// will rewrite the file everytime
	f = fopen(fname, "w");
	if(f == NULL){
		perror(errstr);
		printf("Error opening file.\n");
		exit(1);
	}
	
	pid_t (*cpids)[spawnNum] = malloc(sizeof *cpids);
	pcpids = cpids;
	// start forking off processes
	for(i = 0; i < spawnNum; i++){
		(*cpids)[i] = fork();
		if ((*cpids)[i] < 0) {
			perror(errstr); 
			printf("Fork failed!\n");
			exit(1);
		}
		else if ((*cpids)[i] == 0){
			// pass to the execlp the name of the code to exec
			execlp("child", "child", NULL);
			perror(errstr); 
			printf("execl() failed!\n");
			exit(1);
		}
	}
		
	// calculate end time
	start = time(NULL);
	endwait = start + timeToWait;
	fprintf(f, "Master: Starting clock loop at %s.\n", ctime(&start));
	
	// master waits until the real or simulated time limit runs out or 
	// 100 kids have been spawned in total
    while (1) {  
		int who;
		
		// increment the clock
		start = time(NULL);
		clock[1] += clockInc;
		if(clock[1] - 1000000000 > 0){
			clock[1] -= 1000000000; 
			clock[0] += 1;
		}
		
		// check to see if shmMsg is empty entirely
		// if it is, start loop again
		// else, write to file using the information in it and then clear it
		if(shmMsg[0] != 0 || shmMsg[1] != 0 || shmMsg[2] != 0){
			// check who sent this message so we can overwrite it as it's
			// already dead when we get here
			for(i = 0; i < spawnNum; i++){
				if((*cpids)[i] == shmMsg[0]){
					who = i;
				}
			}
			printf("Master: Child %d is terminating at Master time %d.%d | Message recieved at %d.%d\n", shmMsg[0], clock[0], clock[1], shmMsg[1], shmMsg[2]);
			fprintf(f, "Master: Child %d is terminating at Master time %d.%d | Message recieved at %d.%d\n", shmMsg[2], clock[0], clock[1], shmMsg[1], shmMsg[2]);
			
			shmMsg[0] = 0;
			shmMsg[1] = 0;
			shmMsg[2] = 0;
			activeKid--;					// remove an active kid
			printf("Master: %i active kids.\n", activeKid);
			
			// we need to make another and overwrite the previous entry in the
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
		sleep(3);
		
	}
	
	start = time(NULL);
	fprintf(f, "Master: Ending clock loop at %s", ctime(&start));
	fprintf(f, "Master: Simulated time ending at: %i seconds, %i nanoseconds.\n", clock[0], clock[1]);
	
	fprintf(f, "Total kids spawned: %i\n", kidLim);
	
	// if we have kids still alive after the loop ends, find them
	// and terminate them
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
	clean_up();

    return 0;
}

/* CLEAN UP */ 
// release all shared memory, malloc'd memory, semaphores and close files.
void clean_up(){
	int i;
	printf("Master: Cleaning up now.\n");
	shmctl(shmidA, IPC_RMID, NULL);
	shmctl(shmidB, IPC_RMID, NULL);
	fclose(f);
	if (sem_unlink(SEM_NAME) < 0){
		perror("sem_unlink(3) failed");
	}
	free(pcpids);
	sem_close(semaphore);
}

/* SIGNAL HANDLER */
// catches SIGINT (Ctrl+C) and notifies user that it did.
// it then cleans up and ends the program.
void sig_handler(int signo){
	if (signo == SIGINT){
		printf("Master: Caught Ctrl-C.\n");
		clean_up();
		exit(0);
	}
}
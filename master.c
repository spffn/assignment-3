/* Taylor Clark
CS 4760
Assignment #3
Semaphores and Operating System Simulator 
*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <signal.h>

char errstr[50];

int main(int argc, char *argv[]){
	
	// the processes to create
	int spawnNum = 5;
	int kidLim = 0;
	// 1,000,000,000 ns = 1 seconds
	// increment by 1,000 by default
	int clockInc = 1000;
	pid_t pid = (long)getpid();
	
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
	while ((c = getopt(argc, argv, ":t:hl::s::i::")) != -1) {
		switch (c) {
			// set the clock increment in nanoseconds
			case 'i': {
				clockInc = atoi(optarg);
				printf ("Setting clock increment to: %i nanoseconds\n", spawnNum);
				break;
			}
			// set the number of slave processes to spawn
			case 's': {
				spawnNum = atoi(optarg);
				printf ("Number of chidren to spawn set to: %i\n", spawnNum);
				break;
			}
			// sets the amount of time to let a program run until termination
			case 't': {
				timeToWait = atoi(optarg);
				printf ("Program run time set to: %i seconds\n", timeToWait);
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
			// show help
			case 'h': {
				printf("\n----------\n");
				printf("HELP LIST: \n");
				printf("-i: \n");
				printf("\t Sets the simulation clock increment in nanoseconds.\n");
				printf("\t Default is 60,000,000 nanoseconds.\n");
				printf("\tex: -i 14000000 \n");
				
				printf("-h: \n");
				printf("\tShow help, valid options and required arguments. \n");
				
				printf("-l: \n");
				printf("\t Sets the name of the log file to output to.\n");
				printf("\t Default is log.out.\n");
				printf("\tex: -l filename \n");
				
				printf("-s: \n");
				printf("\t Sets the number of processes to spawn.\n");
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

	// compute time to end program
	
	// SHARED MEMORY
	key = 1001;
	// shared memory clock
	// [0] is seconds, [1] is nanoseconds
	int *clock;
	
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
		
	// write to shared memory the intial clock settings
	clock[0] = 0;
	clock[1] = 0;

	// start forking off processes
	for(i = 0; i < spawnNum; i++){
		pid = fork();
		if (pid < 0) {
			perror(errstr); 
			printf("Fork failed!\n");
			exit(1);
		}
		else if (pid == 0){
			// pass to the execlp the name of the code to exec
			execlp("child", "child", NULL);
			perror(errstr); 
			printf("execl() failed!\n");
			exit(1);
		}
	}
		
	// master waits until the real or simulated time limit runs out or 100 kids have been spawned
	start = time(NULL);
	endwait = start + timeToWait;
	printf("Master: Starting clock loop at %s.\n", ctime(&start));
    while ((start < endwait) && (clock[0] < 2))
    {  
		start = time(NULL);
		clock[1] += clockInc;
		if(clock[1] - 1000000000 > 0){
			clock[1] -= 1000000000; 
			clock[0] += 1;
		}
		kidLim++;
	}
	
	start = time(NULL);
	printf("Master: Ending clock loop at %s", ctime(&start));
	printf("Master: Simulated time ending at: %i seconds, %i nanoseconds.\n", clock[0], clock[1]);
	
	shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
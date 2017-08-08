/* ------------------------------------------------------------ */
/*	ITS60503 - Assignment 1 									*/
/*	Operating Systems 											*/
/*	Student Name: Heng Hian Wee									*/
/*	Student ID: 0321343											*/
/* ------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>		/* exit */
#include <string.h>		/* strcpy, strcat, strtok */
#include <time.h>		/* time_t, time, strftime (for timestamp not including microsecs) */
#include <sys/time.h>	/* gettimeofday, timeval (for timestamp in microsecs) */
#include <sys/types.h>	/* pid_t */
#include <unistd.h>		/* fork */
#include <sys/wait.h>	/* wait */
#include <sys/stat.h>	/* mkfifo */
#include <fcntl.h>		/* open */

#define MAX_LENGTH 128
#define MY_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

typedef struct Message {
	int index;					/* to be separated, not entered into the log */
	char content[MAX_LENGTH];	/* contains message per line from 'readMsg.txt' for log */
} msg;

typedef struct timestamp_struct {
	char timestamp_1[50]; 	/* contains date and time, without microseconds */
	long int microseconds;	/* contains microseconds */
} timestamp_struct;

int main() {
	timestamp_struct thisTime;
	time_t currentTime;

	FILE *parentR;	/* to be used by parent to read 'readMsg.txt' */
	FILE *parentL;	/* to be used for logging parent process */
	FILE *child01L;	/* to be used for logging child01 process */
	FILE *child02L; /* to be used for logging child02 process */
	FILE *child03L; /* to be used for logging child03 process */

	int pipe01[2];	/* file descriptors for unnamed pipe, parent <-> child01 */
	int pipe02[2];	/* file descriptors for unnamed pipe, child01 <-> child02 */
	int pipe03[2];	/* file descriptors for unnamed pipe, parent <-> child03 */
	int pipeR;		/* file descriptor for named pipe to read, child02 <-> child03 */
	int pipeW;		/* file descriptor for named pipe to write, child02 <-> child03 */

	msg messageContain;				/* contain contents of 'readMsg.txt' without index */
	char tempBufferP[MAX_LENGTH];	/* contain contents of 'readMsg.txt' with index */
	char tempBuffer1[MAX_LENGTH];	/* contain contents from pipe01 (parent <-> child01) */
	char tempBuffer2[MAX_LENGTH];	/* contain contents from pipe02 (child01 <-> child02) */
	char tempBuffer3[MAX_LENGTH];	/* contain contents from named pipe (child02 <-> child03) */
	char tempBuffer4[MAX_LENGTH];	/* contain contents from pipe03 (parent <-> child03) */

	pid_t child01;	/* store child01's process id */
	pid_t child02;	/* store child02's process id */
	pid_t child03;	/* store child03's process id */

	/* create unnamed pipe, parent <-> child01 */
	if(pipe(pipe01) == -1) {
		perror("Failed to open pipe01");
		exit(1);
	}

	/* create unnamed pipe, child01 <-> child02 */
	if(pipe(pipe02) == -1) {
		perror("Failed to open pip02");
		exit(1);
	}

	/* create unnamed pipe, parent <-> child03 */
	if(pipe(pipe03) == -1) {
		perror("Failed to open pipe03");
		exit(1);
	}

	/* create named pipe, child02 <-> child03 */
	unlink("./myfifo");						/* delete myfifo if exists */
	if(mkfifo("./myfifo", MY_PERMS))		/* create myfifo unless it exists */
		perror("Failed to create './myfifo', it may already exist");

	/* create child01 process */
	switch(child01 = fork()) {
		case -1:
			perror("Failed to fork child01");
			exit(2);

		case 0:
			/* currently in child01 process */
			switch(child02 = fork()) {
				case -1:
					perror("Failed to fork child02");
					exit(3);

				case 0:
					/* currently in child02 process */
					close(pipe01[0]);	/* close read from pipe01 */
					close(pipe01[1]);	/* close write to pipe01 */
					close(pipe02[1]);	/* close write to pipe02 */

					/* open './myfifo' for writing */
					pipeW = open("./myfifo", O_WRONLY);
					if(pipeW == -1) {
						perror("Cannot open './myfifo' for writing");
						exit(4);
					}
					puts("Opened './myfifo' for writing.");

					/* record process in child02 log */
					child02L = fopen("child02.log", "a+");
					if(!child02L)
						printf("A problem has occured with 'child02.log'\n");
					else 
						/* read from pipe02, write into myfifo */
						while(read(pipe02[0], tempBuffer2, MAX_LENGTH) != 0) {
							char *token2;		/* used in separating the buffer string */
							msg pipe02Contain;	/* contains content ready for comparison */

							token2 = strtok(tempBuffer2, "\t");		/* separate index */
							pipe02Contain.index = atoi(token2);		/* retrieve index as int */
							token2 = strtok(NULL, "\n");			/* obtain message content */
							strcpy(pipe02Contain.content, token2);

							/* record process in child02 log - decide if KEEP or FORWARD */
							currentTime = time(NULL);
							struct tm *tm = localtime(&currentTime);
							strftime(thisTime.timestamp_1, sizeof(thisTime.timestamp_1), "%d/%m/%Y %H:%M:%S", tm);

							struct timeval microsecs;
							gettimeofday(&microsecs, NULL);
							thisTime.microseconds = microsecs.tv_usec;

							if(pipe02Contain.index < 2)			/* if index < 2, the message may not have been kept in a previous process */
								printf("There may have been some messages meant to be kept in a previous process, but are currently in the buffer.\n");
							else if(pipe02Contain.index == 2)	/* if index == 2, the message is to be kept by this process */
								fprintf(child02L, "%s.%li\t%s\tKEEP\n", thisTime.timestamp_1, thisTime.microseconds, pipe02Contain.content);
							else {								/*otherwise, pass it on to the next process */
								fprintf(child02L, "%s.%li\t%s\tFORWARD\n", thisTime.timestamp_1, thisTime.microseconds, pipe02Contain.content);
								strcat(tempBuffer2, "\t");					/* put back format of message with index */
								strcat(tempBuffer2, pipe02Contain.content);	/* put back content of message with index */
								write(pipeW, tempBuffer2, MAX_LENGTH);
							}
						}
					
					fclose(child02L);

					if(unlink("./myfifo") == -1)
						perror("Failed to remove './myfifo'");
					else
						printf("Unlinked './myfifo'\n");

					close(pipe02[0]);	/* close read from pipe02 */
					close(pipeW);		/* close write from named pipe */

					printf("I am child02 (pid %d)\n", getpid());
					wait(NULL);	/* wait for child process to join with parent */
					exit(0);
					break;

				default:
					/* still in, or returned to child01 process */
					close(pipe01[1]);	/* close write to pipe01 */
					close(pipe02[0]);	/* close read from pipe02 */

					/* record process in child01 log */
					child01L = fopen("child01.log", "a+");
					if(!child01L)
						printf("A problem has occured with 'child01.log'\n");
					else 
						/* read from pipe01, write to pipe02 */
						while(read(pipe01[0], tempBuffer1, MAX_LENGTH) != 0) { /* read each entry one by one from pipe01 */
							char *token1;		/* used in separating the buffer string */
							msg pipe01Contain;	/* contains content ready for comparison */

							token1 = strtok(tempBuffer1, "\t");		/* separate index */
							pipe01Contain.index = atoi(token1); 	/* retrieve index as int */
							token1 = strtok(NULL, "\n"); 			/* obtain message content */
							strcpy(pipe01Contain.content, token1);

							/* record process in child01 log - decide if KEEP or FORWARD */
							currentTime = time(NULL);
							struct tm *tm = localtime(&currentTime);
							strftime(thisTime.timestamp_1, sizeof(thisTime.timestamp_1), "%d/%m/%Y %H:%M:%S", tm);

							struct timeval microsecs;
							gettimeofday(&microsecs, NULL);
							thisTime.microseconds = microsecs.tv_usec;

							if(pipe01Contain.index == 1)	/* if index == 1, the message is to be kept by this process */
								fprintf(child01L, "%s.%li\t%s\tKEEP\n", thisTime.timestamp_1, thisTime.microseconds, pipe01Contain.content);
							else {							/* otherwise, pass it on to the next process */
								fprintf(child01L, "%s.%li\t%s\tFORWARD\n", thisTime.timestamp_1, thisTime.microseconds, pipe01Contain.content);
								strcat(tempBuffer1, "\t");					/* put back format of message with index */
								strcat(tempBuffer1, pipe01Contain.content);	/* put back content of message with index */
								write(pipe02[1], tempBuffer1, MAX_LENGTH);
							}
						}

					fclose(child01L);

					close(pipe01[0]);	/* close read from pipe01 */
					close(pipe02[1]);	/* close write to pipe02 */

					printf("I am child01 (pid %d), child02 (pid %d)\n", getpid(), child02);
					wait(NULL); /* wait for child process to join with parent */
					exit(0);
			}
		
			break;

		default:
			/* currently in parent process */
			close(pipe01[0]);	/* close read from pipe01 */

			/* read from 'readMsg.txt' */
			parentR = fopen("readMsg.txt", "r");
			if(!parentR)
				printf("'readMsg.txt' might not exist in the current directory.\n");
			else
				/* write contents into pipe01 */
				while(fgets(tempBufferP, MAX_LENGTH, parentR))
					write(pipe01[1], tempBufferP, MAX_LENGTH);
				rewind(parentR); /* set parentR pointer back to beginning of 'readMsg.txt' */

				/* record process in parent log*/
				while(!feof(parentR)) {
					fscanf(parentR, "%d\t%[^\n]\n", &messageContain.index, messageContain.content);

					/* obtaining first part of timestamp */
					currentTime = time(NULL);
					struct tm *tm = localtime(&currentTime);
					strftime(thisTime.timestamp_1, sizeof(thisTime.timestamp_1), "%d/%m/%Y %H:%M:%S", tm);

					/* obtain microseconds - meant for easier comparison in logs */
					struct timeval microsecs;
					gettimeofday(&microsecs, NULL);
					thisTime.microseconds = microsecs.tv_usec;

					/* This line of code below will print out whatever that is recorded into the log. */
					/* printf("%s.%li\t%s\n", thisTime.timestamp_1, thisTime.microseconds, messageContain.content); */
					
					/* write into parent.log */
					parentL = fopen("parent.log", "a+");
					if(!parentL)
						printf("A problem has occured with 'parent.log'\n");
					else	/* pass messages to next process */
						fprintf(parentL, "%s.%li\t%s\tFORWARD\n", thisTime.timestamp_1, thisTime.microseconds, messageContain.content);
					fclose(parentL);					
				}
			
			fclose(parentR);
			close(pipe01[1]);	/* close write to pipe01 */

			printf("I am the parent (pid %d), child01 (pid %d)\n", getpid(), child01);

			/* create child03 process */
			switch(child03 = fork()) {
				case -1:
					perror("Failed to fork child03");
					exit(3);

				case 0:
					/* currently in child 03 process */
					close(pipe03[0]);	/* close read from pipe03 */
					close(pipe02[0]);	/* close read from pipe02 */
					close(pipe02[1]);	/* close write to pipe02 */
					close(pipe01[0]);	/* close read from pipe01 */
					close(pipe01[1]);	/* close write to pipe01 */

					/* open './myfifo' for writing */
					pipeR = open("./myfifo", O_RDONLY);
					if(pipeR == -1) {
						perror("Cannot open './myfifo' for reading");
						exit(4);
					}
					puts("Opened './myfifo' for reading.");

					/* record process in child03 log */
					child03L = fopen("child03.log", "a+");
					if(!child03L)
						printf("A problem has occured with 'child03.log'\n");
					else
						/* read from './myfifo', write into pipe03 */
						while(read(pipeR, tempBuffer3, MAX_LENGTH) != 0) {
							char *token3;		/* used in separating the buffer string */
							msg pipe03Contain;	/* contains content ready for comparison */

							token3 = strtok(tempBuffer3, "\t");		/* separate index */
							pipe03Contain.index = atoi(token3);		/* retrieve index as int */
							token3 = strtok(NULL, "\n");			/* obtain message content */
							strcpy(pipe03Contain.content, token3);

							/* record process in child03 log - decide if KEEP or FORWARD */
							currentTime = time(NULL);
							struct tm *tm = localtime(&currentTime);
							strftime(thisTime.timestamp_1, sizeof(thisTime.timestamp_1), "%d/%m/%Y %H:%M:%S", tm);

							struct timeval microsecs;
							gettimeofday(&microsecs, NULL);
							thisTime.microseconds = microsecs.tv_usec;

							if(pipe03Contain.index < 3)			/* if index < 3, the message may have not been kept in a previous process */
								printf("There may have been some messages meant to be kept in a previous process, but are currently in the buffer.\n");
							else if(pipe03Contain.index == 3)	/* if index == 3, the message is to be kept by this process */
								fprintf(child03L, "%s.%li\t%s\tKEEP\n", thisTime.timestamp_1, thisTime.microseconds, pipe03Contain.content);
							else {								/* otherwise, pass it on to the next process */
								fprintf(child03L, "%s.%li\t%s\tFORWARD\n", thisTime.timestamp_1, thisTime.microseconds, pipe03Contain.content);
								strcat(tempBuffer3, "\t");					/* put back format of message with index */
								strcat(tempBuffer3, pipe03Contain.content);	/* put back content of message with index */
								write(pipe03[1], tempBuffer3, MAX_LENGTH);
							}
						}
					
					fclose(child03L);
					
					close(pipe03[1]); 	/* close write to pipe03 */
					close(pipeR);		/* close read from named pipe */

					printf("I am child03 (pid %d)\n", getpid());
					wait(NULL);	/* wait for child process to join with parent */
					exit(0);
					break;

				default:
					/* currently in parent process, again */
					close(pipe03[1]);	/* close write to pipe03 */
					close(pipe02[0]);	/* close read from pipe02 */
					close(pipe02[1]);	/* close write from pipe02 */
					close(pipe01[0]);	/* close read from pipe01 */
					close(pipe01[1]);	/* close write from pipe01 */

					/* record process in parent log */
					parentL = fopen("parent.log", "a+");
					if(!parentL)
						printf("A problem has occured with 'parent.log'\n");
					else
						while(read(pipe03[0], tempBuffer4, MAX_LENGTH) != 0) {
							char *token4;		/* used in separating the buffer string */
							msg pipe04Contain;	/* contains content ready for comparison */

							token4 = strtok(tempBuffer4, "\t");		/* separate index */
							pipe04Contain.index = atoi(token4);		/* retrieve index as int */
							token4 = strtok(NULL, "\n");			/* obtain message content */
							strcpy(pipe04Contain.content, token4);

							/* record process in parent log - decide if KEEP or FORWARD */
							currentTime = time(NULL);
							struct tm *tm = localtime(&currentTime);
							strftime(thisTime.timestamp_1, sizeof(thisTime.timestamp_1), "%d/%m/%Y %H:%M:%S", tm);

							struct timeval microsecs;
							gettimeofday(&microsecs, NULL);
							thisTime.microseconds = microsecs.tv_usec;

							if(pipe04Contain.index < 4)		/* if index < 4, the message may have been kept in a previous process */
								printf("There may have been some messages meant to be kept in a previous process, but are currently in the buffer.\n");
							else							/* otherwise, keep the message */
								fprintf(parentL, "%s.%li\t%s\tKEEP\n", thisTime.timestamp_1, thisTime.microseconds, pipe04Contain.content);
						}
					fclose(parentL);
					close(pipe03[0]);	/* close read from pipe03 */
					printf("I am the parent (pid %d), child03 (pid %d)\n", getpid(), child03);
					wait(NULL);	/* wait for child processes to join with parent */
					exit(0);
			}

	}
	exit(0);
	return 0;
}

/* ------------------------------------------------------------ */
/*	End of Program			 									*/
/* ------------------------------------------------------------ */




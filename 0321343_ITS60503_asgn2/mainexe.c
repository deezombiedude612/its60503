/* ------------------------------------------------------------ */
/*	ITS60503 - Assignment 2										*/
/*	Operating Systems 											*/
/*	Student Name: Heng Hian Wee									*/
/*	Student ID: 0321343											*/
/* ------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_SIZE 1024		/* buffer size */
#define MAX_LINES 16			/* maximum number of lines to be read */

/* function prototypes */
void *readFile();
void *writeFile();

FILE *readPtr;		/* file pointer to input file for reading */
FILE *writePtr;		/* file pointer to output file for writing */

char *sharedBuffer[MAX_LINES] = {0};		/* shared buffer */

pthread_mutex_t mutexA = PTHREAD_MUTEX_INITIALIZER;	/* mutex for read thread(s) */
pthread_mutex_t mutexB = PTHREAD_MUTEX_INITIALIZER;	/* mutex for write thread(s) */

int endOfFileMou = 0;	/* indicates whether end of input file has been read */
int linesFilled = 0;	/* indicates how many lines in the shared buffer are filled (readFile()) */
int linesRead= 0;		/* indicates how many lines in the shared buffer had been read (writeFile()) */

int main(int argc, char *argv[]) {
	/* 
		Command must follow required format!
		Checks whether command contains 4 argument vectors.
		Command example: ./main in.txt out.txt 5
		argv[0] = ./main 	executable file
		argv[1] = in.txt	input text file
		argv[2] = out.txt 	output text file
		argv[3] = 5			number of threads for each type to be created
	*/
	if(argc != 4) {	/* terminate program if UNIX command does not follow required format */
		printf("\nWrong input command format.\nFollow the format:\n\n\t$ %s <input_file> <output_file> <number_of_threads>\n\n", argv[0]);
		return 1;
	}

	/* check if input and output files can be opened */
	readPtr = fopen(argv[1], "r");
	writePtr = fopen(argv[2], "w");
	if(!readPtr || !writePtr) {		/* if input file cannot be opened or output file cannot be created */
		printf("\n");				/* terminate immediately */
		if(!readPtr)
			printf("Unfortunately, '%s' cannot be opened.\nCheck and see if your input file is missing.\n", argv[1]);
		if(!writePtr)
			printf("Unfortunately, '%s' cannot be opened.\n", argv[2]);
		printf("\n");
		exit(2);
	}

	int numThreads = atoi(argv[3]);		/* get number of threads to be created */
	if(numThreads < 1) {
		printf("\nUnable to handle with less than 1 thread.\n\n");
		exit(3);
	}

	int a, b, c, d;						/* counters for creating and joining threads */
	pthread_t rThread[numThreads];		/* reader threads */
	pthread_t wThread[numThreads];		/* writer threads */

	/* creating threads */
	for(a = 0; a < numThreads; a++)							/* creating reader threads */
		pthread_create(&rThread[a], NULL, &readFile, NULL);
	for(b = 0; b < numThreads; b++)							/* creating writer threads*/
		pthread_create(&wThread[b], NULL, &writeFile, NULL);

	/* joining threads */
	for(c = 0; c < numThreads; c++)			/* joining reader threads */
		pthread_join(rThread[c], NULL);
	for(d = 0; d < numThreads; d++)			/* joining writer threads */
		pthread_join(wThread[d], NULL);

	fclose(readPtr);	/* close reader file pointer */
	fclose(writePtr);	/* close writer file pointer */

	/* Alert message: successful thread transfer operations */
	printf("\nAll content has been successfully copied..");
	printf("\nfrom:\t%s", argv[1]);
	printf("\nto:\t%s", argv[2]);
	printf("\n\nPlease check whether the input file and output file are identical:");
	printf("\n\n\t$ diff %s %s", argv[1], argv[2]);
	printf("\n\techo $?\t\t--> should produce 0.\n\n");

	return 0;
}

/* reads content from input file, places it into the shared buffer */
void *readFile() {
	char readBuffer[BUFFER_SIZE];	/* input file content - carries one line at a time */

	/* infinite loop, to be broken once all lines have been read and placed into the shared buffer */
	while(1) {
		pthread_mutex_lock(&mutexA);	/* lock current thread */
		if(feof(readPtr)) {					/* if nothing else can be read from the input file */
			endOfFileMou = 1;				/* set end of file flag as TRUE */
			pthread_mutex_unlock(&mutexA);	/* unlock current thread */
			pthread_exit(0);				/* terminate thread */
			break;
		}
		
		/* if there are still lines to fill in the shared buffer */
		if(linesFilled != 16) {
			if(fgets(readBuffer, BUFFER_SIZE, readPtr)) {	/* so long as there is content on the current line in the input file */
				/* printf("%s", readBuffer); */

				/* allocate just enough memory for the current line to take in the shared buffer */
				sharedBuffer[linesFilled] = (char*)malloc(strlen(readBuffer) + 1);		/* allocate enough memory for read buffer array entry */
				strncpy(sharedBuffer[linesFilled], readBuffer, sizeof(readBuffer));		/* transfer read content into the shared buffer */
				/* printf("%s", sharedBuffer[linesFilled]); */							/* checks on what is contained in the shared buffer */
				/* 
					NOTE: Use strlcpy() instead of strncpy() when using MacOS!
					Reference: https://developer.apple.com/library/content/documentation/Security/Conceptual/SecureCodingGuide/Articles/BufferOverflows.html
				*/

				/* reset counter if it hits 15 - repetition meant to make sure all lines are successfully transferred to the shared buffer */
				if(linesFilled < 15)
					linesFilled++;		/* otherwise, increment it by 1 */
				else
					linesFilled = 0;
			}
		}
		pthread_mutex_unlock(&mutexA);	/* unlock current thread */
	}
}

/* retrieves content from shared buffer, writes it to output file */
void *writeFile() {
	/*char writeBuffer[BUFFER_SIZE];	*//* output file content - carries on line at a time */

	/* infinite loop, to be broken once all lines are read from shared buffer and have been transferred to output file */
	while(1) {
		pthread_mutex_lock(&mutexB);	/* lock current thread */

		/* if the shared buffer (current line) still contains content */
		if(sharedBuffer[linesRead]) {
			/*strcpy(writeBuffer, sharedBuffer[linesRead]);
			printf("%s", writeBuffer);*/
			fputs(sharedBuffer[linesRead], writePtr);	/* place current line from shared buffer into output file */
			sharedBuffer[linesRead] = NULL;				/* clear the current line in the shared buffer */
			free(sharedBuffer[linesRead]);				/* free any memory space previously consumed by current line */
			
			/* reset counter if it hits 15 - repetition meant to make sure all lines are read */
			if(linesRead < 15)
				linesRead++;	/* otherwise, increment it by 1 */
			else
				linesRead = 0;
		}
		else if(!sharedBuffer[linesRead] && endOfFileMou == 1) {	/* if current shared buffer line is empty and end of file flag is TRUE*/
			pthread_mutex_unlock(&mutexB);	/* unlock current thread */
			pthread_exit(0);				/* terminate thread */
			break;
		}
		pthread_mutex_unlock(&mutexB);	/* unlock current thread */
	}
}

/* ------------------------------------------------------------ */
/*	END OF PROGRAM			 									*/
/* ------------------------------------------------------------ */




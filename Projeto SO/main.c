#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include "fs.h"
#include <sys/time.h>

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
int numberBuckets = 0;
tecnicofs* fs;
    
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int prodptr = 0;
int consptr = 0;

pthread_mutex_t lock_p, lock_c;
sem_t producer;
sem_t consumer;

void mutex_lock(pthread_mutex_t *lock){
    #if defined (MUTEX) || defined (RWLOCK)
    if (pthread_mutex_lock(lock)){
     perror("Failed to lock mutex\n");
    }
    #endif
}

void mutex_unlock(pthread_mutex_t *lock){
    #if defined (MUTEX) || defined (RWLOCK)
    if (pthread_mutex_unlock(lock)){ 
    	perror("Failed to unlock mutex\n");
    }
    #endif
}

void mutex_destroy_sem(){
    #if defined (RWLOCK) || defined (MUTEX)
        if (pthread_mutex_destroy(&lock_c) != 0){
            perror("mutex_destroy failed\n");  
        }
        if (pthread_mutex_destroy(&lock_p) != 0){
        	perror("mutex_destroy failed\n");	
        }
    #endif
    if (sem_destroy(&producer) != 0){
    	perror("mutex destroy failed\n");
    	
    }
    if (sem_destroy(&consumer) != 0){
    	perror("mutex destroy failed\n"); 	
    }
}

static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numthreads numbuckets\n", appName);
    
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
    numberThreads = atoi(argv[3]);
    numberBuckets = atoi(argv[4]);
}       

int insertCommand(char* data) {
	sem_wait(&producer);
	mutex_lock(&lock_p);
    strcpy(inputCommands[(prodptr++) % MAX_COMMANDS], data);
    mutex_unlock(&lock_p);
    sem_post(&consumer);    
    return 1;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    
}

void *processInput(void *fileName){
    char line[MAX_INPUT_SIZE];
    FILE *inputFile;
	inputFile = fopen(fileName, "r");\
    if (!inputFile){
        printf("\nFile not found."); 
    }

   
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token;
        char arg[MAX_INPUT_SIZE];
        char new_name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %s", &token, arg, new_name);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return NULL;
            case 'r':
                if (numTokens != 3)
                    errorParse();
            	if(insertCommand(line))
                    break;
                return NULL;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    insertCommand("e EOF\n");
    fclose(inputFile);
    return NULL;
}

void *applyCommands(){
    while(1){ 	
        char token;
        char name[MAX_INPUT_SIZE];
        char new_name[MAX_INPUT_SIZE];
        int iNumber;


        sem_wait(&consumer);
		mutex_lock(&lock_c);
		sscanf(inputCommands[(consptr++) % MAX_COMMANDS], "%c %s %s", &token, name, new_name);
		if (token == 'c') iNumber = obtainNewInumber(fs, name);
		mutex_unlock(&lock_c);
        sem_post(&producer);

        int searchResult;
        switch (token)   {
            case 'c':
                create(fs, name, iNumber);
                break;
            case 'l':
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                delete(fs, name);
                break;
            case 'r':
                renameFile(fs, name, new_name);
                break;
            case 'e': {
            	insertCommand("e EOF\n");
            	return NULL;
            }
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");    
                exit(EXIT_FAILURE);      
            }
    	}	    
	}
}

void writeFile(char* fileName){
    FILE *outputFile;

    outputFile = fopen(fileName ,"w");
    if (outputFile == NULL) {
        perror("Error opening output file");
    }
    print_tecnicofs_tree(outputFile, fs);

    fclose(outputFile);
}

void excecuteThreads(void *input){
    pthread_t inputThread;
    #if defined (MUTEX) || defined (RWLOCK)
    pthread_t *tid = malloc(numberThreads * sizeof(pthread_t));
    #endif

    if (pthread_create(&inputThread, NULL, processInput, input) != 0){
        perror("Failed to create thread\n");
    }

   	#if defined (MUTEX) || defined (RWLOCK)
    for (int i=0; i < numberThreads; i++){
        if (pthread_create(&tid[i], NULL, applyCommands, NULL) != 0){
            perror("Failed to create thread\n");
            
        }        
    }
    #else
    applyCommands();
    #endif

    if (pthread_join(inputThread, NULL) != 0){
        perror("Failed to join thread\n");
        
    }
    #if defined (MUTEX) || defined (RWLOCK)
    for (int i=0; i<numberThreads; i++){
        if (pthread_join(tid[i], NULL) != 0){
            perror("Failed to join thread\n");
        }
    }
    free(tid);
    #endif
}

void init_mutex_sem(){
    #if defined (MUTEX) || defined (RWLOCK)
    if (pthread_mutex_init(&lock_c, NULL) != 0)
    {
        perror("Mutex init failed\n");   
    }
    #endif
    if (pthread_mutex_init(&lock_p, NULL) != 0){
        perror("Mutex init failed\n");
        
    }
    if (sem_init(&producer, 0, MAX_COMMANDS) != 0){
        perror("Failed to create semaphore\n");
        
    }
    if (sem_init(&consumer, 0, 0) != 0){
        perror("Failed to create semaphore\n");   
    }
}

double get_time(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec;
}

int main(int argc, char* argv[]) {
    double begin, time_spent; 
    parseArgs(argc, argv);
    init_mutex_sem();
    fs = new_tecnicofs(numberBuckets);

    begin = get_time(); /*start clock*/
    excecuteThreads(argv[1]);
    time_spent = (get_time() - begin)/1000000.0; /*finish clock*/
    printf("TecnicoFS completed in %0.4f seconds.\n", time_spent);

    writeFile(argv[2]);
    mutex_destroy_sem();
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}



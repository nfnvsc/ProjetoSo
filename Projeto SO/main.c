#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "fs.h"
#include <sys/time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t lock;

void thread_lock(){
    #ifdef MUTEX
    if (pthread_mutex_lock(&lock)) printf("Failed to lock mutex\n");
    #elif RWLOCK
    if (pthread_mutex_lock(&lock)) printf("Failed to lock mutex\n");
    #endif
}

void thread_unlock(){
    #ifdef MUTEX
    if (pthread_mutex_unlock(&lock)) printf("Failed to unlock mutex\n");
    #elif RWLOCK
    if (pthread_mutex_unlock(&lock)) printf("Failed to unlock mutex\n");
    #endif
}

static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numthreads\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
    
    numberThreads = atoi(argv[3]);
}       

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if((numberCommands > 0)){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    //exit(EXIT_FAILURE);
}

void processInput(char* fileName){
    char line[MAX_INPUT_SIZE];
    FILE *inputFile;

    if (!(inputFile = fopen(fileName, "r")))
        printf("\nFile not found.");
    
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);

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
                return;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(inputFile);
}

void *applyCommands(){
    while(numberCommands > 0){
        thread_lock();
        const char* command = removeCommand();

        if (command == NULL){
            return NULL;
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);

        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        int iNumber;

        if (token == 'c') iNumber = obtainNewInumber(fs);

        thread_unlock();

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
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }

        
    }
    return NULL;
    
}

void writeFile(char* fileName){
    FILE *outputFile;

    outputFile = fopen(fileName ,"w");
    print_tecnicofs_tree(outputFile, fs);

    fclose(outputFile);
}

void excecuteThreads(){
    int i;
    pthread_t *tid = malloc(numberThreads * sizeof(pthread_t));

    for (i=0; i < numberThreads; i++){
        if (pthread_create(&tid[i], NULL, applyCommands, NULL) != 0){
            printf("Failed to create thread\n");
            exit(1);
        }
            
    }

    for (i=0; i<numberThreads; i++){
        if (pthread_join(tid[i], NULL) != 0)
            printf("Failed to join thread\n");
    }
    free(tid);
}

void init_mutex(){
    #ifdef MUTEX
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("Mutex init failed\n");
        exit(1);
    }
    #elif RWLOCK
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("Mutex init failed\n");
        exit(1);
    }
    #endif
}

double get_time(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec;
}

void executeCommands(){
    #ifdef MUTEX
    excecuteThreads();
    #elif RWLOCK
    excecuteThreads();
    #else
    applyCommands();
    #endif        
}

int main(int argc, char* argv[]) {
    double begin, time_spent; 

    parseArgs(argc, argv);
    
    init_mutex();

    fs = new_tecnicofs();

    processInput(argv[1]);

    begin = get_time(); /*start clock*/

    executeCommands();
    
    time_spent = (get_time() - begin)/1000000; /*finish clock*/

    printf("TecnicoFS completed in %0.4f seconds.\n", time_spent);

    writeFile(argv[2]);
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}



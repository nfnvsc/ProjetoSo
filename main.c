#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "fs.h"
#include "tecnicofs-api-constants.h"
#include "tecnicofs-client-api.h"
//#include "unix.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 128
#define MAX_CLIENTS 5

int numberThreads = 0;
int numberBuckets = 0;
tecnicofs* fs;
    
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int prodptr = 0;
int consptr = 0;
int sockfd;

pthread_mutex_t lock_p, lock_c;
sem_t producer;
sem_t consumer;

int sockfd, servlen;
struct sockaddr_un serv_addr;

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
    if (argc != 4) {    //args--> tecnicofs nomesocket outputfile numbuckets
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
    numberBuckets = atoi(argv[3]);
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

/*
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

        perform minimal validation
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
            default: {  error 
                errorParse();
            }
        }
    }
    insertCommand("e EOF\n");
    fclose(inputFile);
    return NULL;
}
*/

void *applyCommands(char *line, int user){ 	
    char token;
    char fileName[MAX_INPUT_SIZE];
    char new_name[MAX_INPUT_SIZE];
	//sscanf(inputCommands[(consptr++) % MAX_COMMANDS], "%c %s %s %s %s", &token, fileName, arg1, arg2, ar3);

    int searchResult;
    switch (token)   {
        case 'c':
            //create(fs, fileName, user, arg1, arg2);
            break;
        case 'l':
            searchResult = lookup(fs, fileName);
            if(!searchResult)
                printf("%s not found\n", fileName);
            else
                printf("%s found with inumber %d\n", fileName, searchResult);
            break;
        case 'd':
            //delete(fs, fileName, user);
            break;
        case 'r':
            //renameFile(fs, fileName, new_name, user);
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");    
            exit(EXIT_FAILURE);      
        }
    }	
    return NULL;
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
/*
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
*/
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

void *str_echo(void *sockfd){
    int n;
    char line[MAX_INPUT_SIZE];
    //char output[MAX_INPUT_SIZE];

    for (;;){
        /* Lê uma linha do socket */
        n = read(*(int*)sockfd, line, MAX_INPUT_SIZE);
        if (n == 0) return NULL;
        else if (n < 0) perror("str_echo: readline error");
        //else output = applyCommands(line. *(int*)sockfd);

        /*Reenvia a linha para o socket. n conta com o \0 da string,
        caso contrário perdia-se sempre um caracter!*/
        if(write(*(int*)sockfd, line, n) != n)
            perror("str_echo:write error");
        printf("%s\n", line);
    }
}

void mountSocket(char* socketName){
    //int servlen;
    //struct sockaddr_un serv_addr;

    /* Cria socket stream */
    if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0) ) < 0)
        perror("server: can't open stream socket");

    /* Elimina o nome, para o caso de já existir.*/
    unlink(socketName);
    
    /* O nome serve para que os clientes possam identificar o servidor */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socketName);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
        perror("server, can't bind local address");

    listen(sockfd, 5);
}

void receiveClients(){
    struct sockaddr_un cli_addr;
    int newsockfd, i;
    socklen_t clilen;
    pthread_t *clientThreads = malloc(MAX_INPUT_SIZE * sizeof(pthread_t));

    for (i = 0; i < MAX_CLIENTS; i = (i + 1) % MAX_CLIENTS){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) perror("server: accept error");
        
        if (pthread_create(&clientThreads[i], NULL, str_echo, (void *) &newsockfd) != 0){
            perror("Failed to create thread\n");
        }
        
       // close(newsockfd);
    }
}

int main(int argc, char* argv[]) {
    //TIMER_T beginTime, endTime; 
    parseArgs(argc, argv);
    init_mutex_sem();
    fs = new_tecnicofs(numberBuckets);
    mountSocket(argv[1]);
    receiveClients();

    /*
    TIMER_READ(beginTime); start clock
    excecuteThreads(argv[1]);
    TIMER_READ(endTime); start clock
    printf("TecnicoFS completed in %0.4f seconds.\n", TIMER_DIFF_SECONDS(beginTime, endTime));
    */
    writeFile(argv[2]);
    mutex_destroy_sem();
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}



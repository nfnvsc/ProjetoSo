#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "fs.h"

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
    printf("Usage: %s socketname outputfile numbuckets\n", appName);
    
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
        exit(0);
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

void wr_int(char* buffer, int d){
    char aux[2];
    sprintf(aux, "%d", d);
    strcpy(buffer, aux);
}

void applyCommands(char *line, int user, open_file* file_table, char* buffer){ 	
    char token, arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
    char aux[MAX_INPUT_SIZE];
    int permissions, return_val;

    sscanf(line, "%c %s %[^\t\n]", &token, arg1, arg2);
    //printf("BUFFER AC_IN: %s\n", buffer);
    switch (token)   {
        case 'c':
            permissions = atoi(arg2); /*permissions = (int) ab
                                                        a = ownerpermissions                                                
                                                        b = othersPermissions*/
            return_val = create(fs, arg1, user, permissions / 10, permissions % 10); //arg1 = filename
            wr_int(buffer, return_val);
            break;
        case 'd':
            return_val = delete(fs, file_table, arg1, user); //arg1 = filename
            wr_int(buffer, return_val);
            break;
        case 'r':
            return_val = renameFile(fs, arg1, arg2, user);   //arg1 = filenameOld, arg2 = filenameNew
            wr_int(buffer, return_val);
            break;
        case 'o':
            return_val = openFile(fs, file_table, arg1, atoi(arg2)/10, user);   //arg1 = filenameOld, arg2 = filenameNew
            wr_int(buffer, return_val);
            break;
        case 'x':
            return_val = closeFile(file_table, atoi(arg1));   //arg1 = filenameOld, arg2 = filenameNew
            wr_int(buffer, return_val);
            break;
        case 'l':
            return_val = readFile(fs, file_table, atoi(arg1), aux, atoi(arg2));   //arg1 = filenameOld, arg2 = filenameNew
            wr_int(buffer, return_val);

            if (return_val == 0){ //if succesful
                strcat(buffer, " ");
                strcat(buffer, aux);
            }

            break;
        case 'w':
            printf("ARG2: %s %ld\n", arg2, strlen(arg2)); //ARG2 NAO CONTEM TUDO
            return_val = writeFileContents(fs, file_table, atoi(arg1), arg2, strlen(arg2));   //arg1 = filenameOld, arg2 = filenameNew
            wr_int(buffer, return_val);
            break;
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");    
            exit(EXIT_FAILURE);      
        }
    }	
    //printf("BUFFER AC_OUT: %s\n", buffer);

}

void write_output_file(char* fileName){
    FILE *outputFile;

    outputFile = fopen(fileName ,"w");
    if (outputFile == NULL) {
        perror("Error opening output file");
    }
    print_tecnicofs_tree(outputFile, fs);

    fclose(outputFile);
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

void *str_echo(void *sockfd){
    int n;
    socklen_t len;
    char line[MAX_INPUT_SIZE];
    char buffer[MAX_INPUT_SIZE]; //max input size?

    open_file* file_table = init_open_file_table();

    for (;;){
        /* Lê uma linha do socket */
        n = read(*(int*)sockfd, line, MAX_INPUT_SIZE);
        printf("\nRECEIVED: %s\n", line);
        if (n == 0) return NULL;
        else if (n < 0) perror("str_echo: readline error");
        else{
            struct ucred ucred;
            len = sizeof(struct ucred);

            if (getsockopt(*(int*)sockfd, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1){
                perror("str_echo: getUID error");
            }
            //buffer = 
            applyCommands(line, ucred.pid, file_table, buffer);
            n = strlen(buffer);
        }
        printf("BUFFER_STR_ECHO:%s\n",buffer);
        /*Reenvia a linha para o socket. n conta com o \0 da string,
        caso contrário perdia-se sempre um caracter!*/
        if(write(*(int*)sockfd, &buffer, n) != n)
            perror("str_echo:write error");
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
    write_output_file(argv[2]);
    mutex_destroy_sem();
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}



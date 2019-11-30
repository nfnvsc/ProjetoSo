#include "tecnicofs-client-api.h"
#include <assert.h>

#define MAXLINE 128

int sockfd, servlen;
struct sockaddr_un serv_addr;

int tfsMount(char *adress){

    /* Cria socket stream */
    if ((sockfd= socket(AF_UNIX, SOCK_STREAM, 0) ) < 0)
        perror("client: can't open stream socket");

    /* Primeiro uma limpeza preventiva */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    /* Dados para o socket stream: tipo + nome que
    identifica o servidor */
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, adress);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    if(connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
        perror("client: can't connect to server");

    return 0;
    /* Envia as linhas lidas do teclado para o socket */
    //str_cli(stdin, sockfd);
}

int tfsUnmount(){
    /* Fecha o socket e termina */
    close(sockfd);
    exit(0);
}

int sendMessage(char* sendline){
    int n;
    char recvline[MAXLINE+1];

    /* Envia string para sockfd.
    Note-se que o \0 não é enviado */
    n = strlen(sendline);


    if (write(sockfd, sendline, n) != n){
        perror("str_cli:write error on socket");
    }
    /* Tenta ler string de sockfd.
    Note-se que tem de terminar a string com \0 */
    n = read(sockfd, recvline, MAXLINE);

    if (n<0) perror("str_cli:readline error");
        recvline[n] = 0;

    return atoi(recvline);
}

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    char sendline[MAXLINE];
    char aux[3];

    strcpy(sendline, "c ");
    strcat(sendline, filename);

    sprintf(aux, " %d", (int)ownerPermissions);
    strcat(sendline, aux);

    sprintf(aux, "%d", (int)othersPermissions);
    strcat(sendline, aux);

    return sendMessage(sendline);
    
}

int tfsDelete(char *filename){
    char sendline[MAXLINE];

    strcpy(sendline, "d ");
    strcat(sendline, filename);

    return sendMessage(sendline);
}

int tfsRename(char *filenameOld, char *filenameNew){
    char sendline[MAXLINE];

    strcpy(sendline, "r ");
    strcat(sendline, filenameOld);
    strcat(sendline, " ");
    strcat(sendline, filenameNew);

    return sendMessage(sendline);
}

int tfsOpen(char *filename, permission mode){
    char sendline[MAXLINE];
    char aux[3];

    strcpy(sendline, "o ");

    strcat(sendline, filename);
    
    sprintf(aux, " %d", (int)mode);
    strcat(sendline, aux);

    return sendMessage(sendline);
}

int tfsClose(int fd){
    char sendline[MAXLINE];
    char aux[3];

    strcpy(sendline, "x ");
    sprintf(aux, "%d", fd);
    strcat(sendline, aux);

    return sendMessage(sendline);
}

int tfsRead(int fd, char *buffer, int len){
    char sendline[MAXLINE];
    char aux[3];

    strcpy(sendline, "l ");
    sprintf(aux, " %d", fd);
    strcat(sendline, aux);
    sprintf(aux, " %d", len);
    strcat(sendline, aux);

    return sendMessage(sendline); 
}

int tfsWrite(int fd, char *buffer, int len){
    char sendline[MAXLINE];
    char aux[3];

    strcpy(sendline, "w ");
    sprintf(aux, " %d", fd); //fix?
    strcat(sendline, aux);

    strncat(sendline, buffer, len);

    return sendMessage(sendline);
}

//TESTE
int main(int argc, char** argv){
    if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    char readBuffer[10] = {0};
    assert(tfsMount(argv[1]) == 0);
    assert(tfsCreate("abc", RW, READ) == 0);
    int fd = -1;
    assert((fd = tfsOpen("abc", RW)) == 0);
    int out = tfsWrite(fd, "12345", 5);
    printf("%d\n", out);
    assert(out == 0);
    
    printf("Test: read full file content");
    assert(tfsRead(fd, readBuffer, 6) == 5);
    printf("Content read: %s\n", readBuffer);
    
    printf("Test: read only first 3 characters of file content");
    memset(readBuffer, 0, 10*sizeof(char));
    assert(tfsRead(fd, readBuffer, 4) == 3);
    printf("Content read: %s\n", readBuffer);
    
    printf("Test: read with buffer bigger than file content");
    memset(readBuffer, 0, 10*sizeof(char));
    assert(tfsRead(fd, readBuffer, 10) == 5);
    printf("Content read: %s\n", readBuffer);

    assert(tfsClose(fd) == 0);

    printf("Test: read closed file");
    assert(tfsRead(fd, readBuffer, 6) == TECNICOFS_ERROR_FILE_NOT_OPEN);

    printf("Test: read file open in write mode");
    assert((fd = tfsOpen("abc", WRITE)) == 0);
    assert(tfsRead(fd, readBuffer, 6) == TECNICOFS_ERROR_INVALID_MODE);


    assert(tfsDelete("abc") == 0);
    assert(tfsUnmount() == 0);

    return 0;

    /*
    int fd;
    char *buffer;
    tfsMount(argv[1]);
    tfsCreate("F1", 1, 2);
    fd = tfsOpen("F1", 1);
    tfsWrite(fd, "cagalhao", 5);
    tfsRead(fd, buffer, 5);
    printf("GGG WWP: %s", buffer);
    tfsUnmount();
    */
}
//TESTE
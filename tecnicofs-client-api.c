#include "tecnicofs-client-api.h"
#include <assert.h>

#define MAXLINE 128

int sockfd, servlen;
struct sockaddr_un serv_addr;

char sendline[MAXLINE];
char recvline[MAXLINE];

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

int sendMessage(){
    int n;
    //char recvline[MAXLINE+1];

    /* Envia string para sockfd.
    Note-se que o \0 não é enviado */
    n = strlen(sendline);

    printf("LINE: %s\n", sendline);

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
    char aux[3];

    strcpy(sendline, "c ");
    strcat(sendline, filename);

    sprintf(aux, " %d", (int)ownerPermissions);
    strcat(sendline, aux);

    sprintf(aux, "%d", (int)othersPermissions);
    strcat(sendline, aux);

    return sendMessage();
    
}

int tfsDelete(char *filename){

    strcpy(sendline, "d ");
    strcat(sendline, filename);

    return sendMessage();
}

int tfsRename(char *filenameOld, char *filenameNew){

    strcpy(sendline, "r ");
    strcat(sendline, filenameOld);
    strcat(sendline, " ");
    strcat(sendline, filenameNew);

    return sendMessage();
}

int tfsOpen(char *filename, permission mode){
    char aux[3];

    strcpy(sendline, "o ");

    strcat(sendline, filename);
    
    sprintf(aux, " %d", (int)mode);
    strcat(sendline, aux);

    return sendMessage();
}

int tfsClose(int fd){
    char aux[3];

    strcpy(sendline, "x ");
    sprintf(aux, "%d", fd);
    strcat(sendline, aux);

    return sendMessage();
}

int tfsRead(int fd, char *buffer, int len){
    char aux[3];
    int output;

    strcpy(sendline, "l ");
    sprintf(aux, "%d", fd);
    strcat(sendline, aux);
    strcat(sendline, " ");
    sprintf(aux, "%d", len);
    strcat(sendline, aux);
    strcpy(buffer, &aux[0]);
    output = sendMessage();
    strncpy(buffer, recvline + 2, strlen(recvline)-1);

    return output;
}

int tfsWrite(int fd, char *buffer, int len){
    char aux[3];

    strcpy(sendline, "w ");
    sprintf(aux, "%d", fd);
    strcat(sendline, aux);
    strcat(sendline, " ");
    strncat(sendline, buffer, len);

    return sendMessage();
}

//TESTE
int main(int argc, char** argv){
    if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    char buffer[100];
    char *teste = "BLA BLA BLAxx";
    tfsMount(argv[1]);
    tfsCreate("TESTE", 3, 3);
    int fd = tfsOpen("TESTE", 3);
    tfsWrite(fd, teste, strlen(teste));
    tfsRead(fd, buffer, strlen(teste));
    printf("%s", buffer);

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
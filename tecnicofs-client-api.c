#include "tecnicofs-client-api.h"

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
    int n, error;
    char recvline[MAXLINE+1];

    /* Envia string para sockfd.
    Note-se que o \0 não é enviado */
    n = strlen(sendline);


    if (error = write(sockfd, sendline, n) != n){
        perror("str_cli:write error on socket");
    }
    /* Tenta ler string de sockfd.
    Note-se que tem de terminar a string com \0 */
    n = read(sockfd, recvline, MAXLINE);

    if (n<0) perror("str_cli:readline error");
        recvline[n] = 0;

    /* Envia a string para stdout */
    fputs(recvline, stdout);

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
    sprintf(aux, " %d", fd);
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
    int fd;
    char *buffer;
    tfsMount(argv[1]);
    tfsCreate("F1", 1, 2);
    fd = tfsOpen("F1", 1);
    tfsWrite(fd, "cagalhao", 5);
    tfsRead(fd, buffer, 5);
    printf("GGG WWP: %s", buffer);
    tfsUnmount();
}
//TESTE
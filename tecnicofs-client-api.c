#include "tecnicofs-client-api.h"
#include <assert.h>

#define MAXLINE 128

int sockfd, servlen, openSession = 0;
struct sockaddr_un serv_addr;

char sendline[MAXLINE];
char recvline[MAXLINE];

int tfsMount(char *adress){
	if (openSession == 1) return TECNICOFS_ERROR_OPEN_SESSION;

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

    openSession = 1;
    return 0;
    /* Envia as linhas lidas do teclado para o socket */
}

int tfsUnmount(){
    /* Fecha o socket e termina */
    if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    if (close(sockfd) != 0) perror("client: error closing socket");
    return 0;    
}

int sendMessage(){
    int n;
    /* Envia string para sockfd.
    Note-se que o \0 não é enviado */
    n = strlen(sendline);

    if (write(sockfd, sendline, n) != n){
        perror("str_cli:write error on socket");
    }
    /* Tenta ler string de sockfd.
    Note-se que tem de terminar a string com \0 */
    n = read(sockfd, recvline, MAXLINE);

    if (n<0) perror("str_cli:readline error on socket");
    
    recvline[n] = '\0';

    return atoi(recvline);
}

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "c %s %d%d",filename, (int)ownerPermissions, (int)othersPermissions);
    return sendMessage();
    
}

int tfsDelete(char *filename){
	if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "d %s",filename);
    return sendMessage();
}

int tfsRename(char *filenameOld, char *filenameNew){
	if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "r %s %s",filenameOld, filenameNew);
    return sendMessage();
}

int tfsOpen(char *filename, permission mode){
	if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "o %s %d",filename, (int)mode);
    return sendMessage();
}

int tfsClose(int fd){
	if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "x %d",fd);
    return sendMessage();
}

int tfsRead(int fd, char *buffer, int len){
    int output;
    if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "l %d %d",fd, len);

    output = sendMessage();

    strncpy(buffer, recvline + 2, strlen(recvline)-1);

    return output;
}

int tfsWrite(int fd, char *buffer, int len){
	if (openSession == 0) return TECNICOFS_ERROR_NO_OPEN_SESSION;
    sprintf(sendline, "w %d %s",fd, buffer);
    return sendMessage();
}
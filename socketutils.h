/*
** Fichero: socketutils.h
** Autores:
** ser365 (https://github.com/ser356)
** andresblz (https://github.com/andresblz)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef SOCKETUTILS_H
#define SOCKETUTILS_H
/*
this file contains the socket utilities
used by the server and client
such as the file log and the question generator function prototypes

FORMAT PROTOCOL FOR THE SERVER AND CLIENT ARE ALSO DEFINED HERE
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define MAX_ATTEMPTS 5
#define HOLA "HOLA\r\n"
#define ADIOS "ADIOS\r\n"
#define ACIERTO "350 ACIERTO\r\n"
#define MAYOR "MAYOR"
#define MENOR "MENOR"
#define TAM_BUFFER 516
#define TEST "TEST\r\n"
#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 1431
#define TIMEOUT 30
#define MAXHOST 512
#define SYNTAX_ERROR "500 Error de sintaxis"
extern int errno;
char *getCurrentTimeStr();
FILE* openLog(char *filename);
int createLog(char *filename);
int esAdios(char *buffer);
char *getAnswerFromIndex(int index, char **matrizPreguntas);
char *getRandomQuestion(char **matrizPreguntas, int *index);
char **readArchivoPreguntas(char *nombreArchivo, int *nlines);
char **readArchivoRespuestas(char *nombreArchivo);
void serverTCP(int s, struct sockaddr_in peeraddr_in, struct sockaddr_in serveraddr_in);
void serverUDP(int s, struct sockaddr_in peeraddr_in, struct sockaddr_in serveraddr_in);
void errout(char *hostname, FILE *log);
void handler();
int recibeUDPMejorado(int s, void *buffer, FILE *logfile,size_t buffer_size, int flags, struct sockaddr *addr, socklen_t *addrlen);
#endif


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

#define HOLA "HOLA\r\n"
#define ADIOS "ADIOS\r\n"
#define ACIERTO "350 ACIERTO\r\n"
#define MAYOR "MAYOR"
#define MENOR "MENOR"
#define SYNTAX_ERROR "500 Error de sintaxis\r\n"

FILE* openLog(char *filename);
int createLog(char *filename);
int esAdios(char *buffer);
char *getAnswerFromIndex(int index, char **matrizPreguntas);
char *getRandomQuestion(char **matrizPreguntas, int *index);
char **readArchivoPreguntas(char *nombreArchivo, int *nlines);
char **readArchivoRespuestas(char *nombreArchivo);

#endif


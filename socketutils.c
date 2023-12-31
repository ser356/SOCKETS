/*
** Fichero: servidor.c
** Autores:
** ser365 (https://github.com/ser356)
** andresblz (https://github.com/andresblz)
*/

#include "socketutils.h"
/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
	FILE *log = openLog("peticiones.log");
	if(log == NULL){
		printf("Error al abrir el archivo de log\n");
		fflush(stdout);
		exit(1);
	} else {
		fprintf(log, "ALARMA RECIBIDA\n");
		fflush(log);
	}	
	fclose(log);
	
}	

char **readArchivoPreguntas(char *nombreArchivo, int *nlines)
{
	*nlines = 0;
	FILE *file = fopen(nombreArchivo, "r");
	if (file == NULL)
	{
		fprintf(stderr, "No se pudo abrir el archivo\n");
		fflush(stdout);

		return NULL;
	}

	char **matrizPreguntas = malloc(10 * sizeof(char *));
	for (int j = 0; j < 10; j++)
		matrizPreguntas[j] = malloc(250 * sizeof(char));

	int i = 0;
	while (fgets(matrizPreguntas[i], 250, file))
	{
		i++;
		*nlines = *nlines + 1;
	}
	fclose(file);
	return matrizPreguntas;
}

char **readArchivoRespuestas(char *nombreArchivo)
{
	FILE *file = fopen(nombreArchivo, "r");
	if (file == NULL)
	{
		fprintf(stderr, "No se pudo abrir el archivo\n");
		fflush(stdout);

		return NULL;
	}

	char **matrizRespuestas = malloc(10 * sizeof(char *));
	for (int j = 0; j < 10; j++)
		matrizRespuestas[j] = malloc(250 * sizeof(char));

	int i = 0;
	while (fgets(matrizRespuestas[i], 250, file))
	{
		i++;
	}
	fclose(file);
	return matrizRespuestas;
}

char *getRandomQuestion(char **matrizPreguntas, int *index)
{

	int random = rand() % 10;
	*index = random;
	return matrizPreguntas[random];
}

char *getAnswerFromIndex(int index, char **matrizPreguntas)
{
	return matrizPreguntas[index];
}

int esAdios(char *buffer)
{
	return strcmp(buffer, ADIOS) == 0 ? 1 : 0;
}

int createLog(char *filename)
{
	FILE *file = fopen(filename, "w");
	// check if exists, if not, create, if exists, append
	if (file == NULL)
	{
		fprintf(stderr, "No se pudo crear el archivo de log\n");
		fflush(stdout);

		return 1;
	}
	else
	{
		fclose(file);
		return 0;
	}
}

FILE *openLog(char *filename)
{
	FILE *file = fopen(filename, "a");
	if (file == NULL)
	{
		printf("No se pudo abrir el archivo de log\n");
		fflush(stdout);
		return NULL;
	}
	return file;
}

char *getCurrentTimeStr()
{
	time_t timevar;
	time(&timevar);
	char *timeStr = ctime(&timevar);
	timeStr[strlen(timeStr) - 1] = '\0'; // Eliminar el carácter de nueva línea
	return timeStr;
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname, FILE *log)
{
	fprintf(log, "Exiting from %s\n", hostname);
	fflush(stdout);
	fclose(log);
	exit(1);
}

int recibeUDPMejorado(int s, void *buffer, FILE *logfile,size_t buffer_size, int flags, struct sockaddr *addr, socklen_t *addrlen)
{
    int n_retry = 0;
    while (n_retry < MAX_ATTEMPTS)
    {
        alarm(TIMEOUT);
        int len = recvfrom(s, buffer, buffer_size, flags, addr, addrlen);
        if (len == -1)
        {
            if (errno == EINTR)
            {
				fprintf(logfile, "Timeout waiting for response from server\n");
				fflush(logfile);
                n_retry++;
                continue;
            }
            else
            {
                perror("recvfrom");
                fprintf(logfile, "Unable to receive response from server\n");
				fflush(logfile);
                fclose(logfile);
				exit(1);
            }
        }
        else
        {
			alarm(0);
            return len;
        }
    }

	fprintf(logfile, "Maximum number of attempts reached\n");
	fflush(logfile);
	fclose(logfile);
    exit(1);
}

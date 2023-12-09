#include "socketutils.h"
/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
	printf("Alarma recibida \n");
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

/*				S E R V R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int sock, struct sockaddr_in clientaddr_in, struct sockaddr_in myaddr_in)
{

	char buf[TAM_BUFFER];	/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST]; /* remote host's name string */

	int len, status;
	// struct hostent *hp; /* pointer to host info for remote host */
	long timevar; /* contains time returned by time() */

	struct linger linger; /* allow a lingering, graceful close; */
						  /* used when setting SO_LINGER */
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */

	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
						 hostname, MAXHOST, NULL, 0, 0);
	if (status)
	{
		/* The information is unavailable for the remote
		 * host.  Just format its internet address to be
		 * printed out in the logging information.  The
		 * address will be shown in "internet dot format".
		 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
	/* Log a startup message. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	/*
		fflush(stdout);
	fflush(stdout);
("Startup from %s port %u at %s",
		   hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

	*/

	/* Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	char *logfile = "peticiones.txt";
	FILE *log = openLog(logfile);
	if (log == NULL)
	{
		// THE ONLY ERROR MSG THAT COULDNT BE LOGGED
		fprintf(stderr, "FATAL!: No se pudo abrir el archivo de log!\n");
		fflush(stdout);

		exit(1);
	}
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger,
				   sizeof(linger)) == -1)
	{
		errout(hostname, log);
	}

	/* Go into a loop, receiving requests from the remote
	 * client.  After the client has sent the last request,
	 * it will do a shutdown for sending, which will cause
	 * an end-of-file condition to appear on this end of the
	 * connection.  After all of the client's requests have
	 * been received, the next recv call will return zero
	 * bytes, signalling an end-of-file condition.  This is
	 * how the server will know that no more requests will
	 * follow, and the loop will be exited.
	 */
	int tries = 5;
	int numberOfLines;
	char **matrizPreguntas = readArchivoPreguntas("archivopreguntas.txt", &numberOfLines);
	fflush(stdout);

	fprintf(log, "===================================================================\n");

	fprintf(log, "STARTING LOG AT %s\n", getCurrentTimeStr());

	fprintf(log, "NUMERO DE LINEAS LEIDAS: %d\n", numberOfLines);
	fprintf(log, "===================================================================\n");

	char **matrizRespuestas = readArchivoRespuestas("archivorespuestas.txt");

	if (matrizPreguntas == NULL)
	{
		fflush(stdout);
		fflush(stdout);
		fprintf(log, "FATAL!: No se pudo leer el archivo de preguntas!\n");
		fflush(stdout);

		exit(1);
	}
	if (matrizRespuestas == NULL)
	{
		fflush(stdout);
		fflush(stdout);
		fprintf(log, "FATAL!: No se pudo leer el archivo de respuestas!\n");
		fflush(stdout);

		exit(1);
	}

	send(sock, "220 SERVICIO PREPARADO\r\n", sizeof("220 SERVICIO PREPARADO\r\n"), 0);

	fprintf(log, "SERVER SENDING at %s on PORT %d|%s|%s|%s|%s", getCurrentTimeStr(), ntohs(clientaddr_in.sin_port), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", "220 SERVICIO PREPARADO\r\n");
	fprintf(log, "===================================================================\n");
	int index;
	int next = 0;
	char *lineofFileofQuestions;

	while (1)
	{

		recv(sock, buf, TAM_BUFFER, 0);

		if (esAdios(buf))
		{

			break;
		}

		if (strcmp(buf, "HOLA\r\n") == 0)
		{

			fprintf(log, "RECEIVED HOLA at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(clientaddr_in.sin_addr), "TCP", buf);
			fprintf(log, "===================================================================\n");
		}

		if (strcmp(buf, HOLA) == 0 || next == 1)
		{

			next = 0;
			lineofFileofQuestions = getRandomQuestion(matrizPreguntas, &index);

			lineofFileofQuestions[strlen(lineofFileofQuestions) - 1] = '\0';

			char response[TAM_BUFFER] = "250 ";
			sprintf(response, "250 %s#%d\r\n", lineofFileofQuestions, tries);
			len = send(sock, response, TAM_BUFFER, 0);

			if (len == -1)
			{
				perror("Error al enviar");
				exit(1);
			}
			fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", response);
			fprintf(log, "===================================================================\n");

			char *respuesta = getAnswerFromIndex(index, matrizRespuestas);
			respuesta[strlen(respuesta) - 1] = '\0';
			strcat(respuesta, "\r\n");

			char esMayoroMenor[TAM_BUFFER];

			do
			{
				// Recibir respuesta del cliente
				sleep(1);
				len = recv(sock, buf, TAM_BUFFER, 0);
				if (len == -1)
				{
					perror("Error al recibir");
					exit(1);
				}
				fprintf(log, "RECEIVED at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(clientaddr_in.sin_addr), "TCP", buf);
				fprintf(log, "===================================================================\n");

				fflush(stdout);

				/*
				LA RESPUESTA VIENE EN FORMATO "RESPUESTA <NUMERO>\r\n"
				POR LO QUE SE DEBE SEPARAR EL NUMERO DEL RESTO DEL STRING
				SINO -> ERROR DE SINTAXIS
				*/
				char evaluar[] = "RESPUESTA ";
				int number;
				int isvalidresponse = sscanf(buf, "%s %d\r\n", evaluar, &number);

				if (strcmp(buf, ADIOS) == 0 || (isvalidresponse == 2) || strcmp(buf, "+\r\n") == 0)
				{ // Verificar si el cliente envió "ADIOS"
					if (strcmp(buf, "+\r\n") == 0)
					{
						tries++;
						sprintf(esMayoroMenor, "345 INCREMENTADO 1 INTENTO\r\n");
						len = send(sock, esMayoroMenor, TAM_BUFFER, 0);
						// dont want to interact with the rest of the code
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", esMayoroMenor);
						fprintf(log, "===================================================================\n");
						continue;
					}
					if (strcmp(buf, "ADIOS\r\n") == 0)
					{
						len = send(sock, ADIOS, sizeof(ADIOS), 0);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						fprintf(log, "CLOSING at %s on PORT %d |%s|%s|%s|%s", getCurrentTimeStr(), ntohs(clientaddr_in.sin_port), hostname, inet_ntoa(clientaddr_in.sin_addr), "TCP", "ADIOS\r\n");
						fprintf(log, "===================================================================\n");
						break;
					}

					if (number > atoi(respuesta))
					{
						tries--;
						sprintf(esMayoroMenor, "354 %s#%d", MENOR, tries);
						strcat(esMayoroMenor, "\r\n");
						len = send(sock, esMayoroMenor, TAM_BUFFER, 0);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", esMayoroMenor);
						fprintf(log, "===================================================================\n");
					}
					if (number < atoi(respuesta))
					{
						tries--;
						sprintf(esMayoroMenor, "354 %s#%d", MAYOR, tries);
						strcat(esMayoroMenor, "\r\n");
						len = send(sock, esMayoroMenor, TAM_BUFFER, 0);
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", esMayoroMenor);
						fprintf(log, "===================================================================\n");
					}
					if (number == atoi(respuesta))
					{
						len = send(sock, ACIERTO, sizeof(ACIERTO), 0);

						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", ACIERTO);
						fprintf(log, "===================================================================\n");
						next = 1;
					}
				}
				else
				{
					tries--;
					sprintf(esMayoroMenor, "%s#%d\r\n", SYNTAX_ERROR, tries);
					len = send(sock, esMayoroMenor, sizeof(esMayoroMenor), 0);
					if (len == -1)
					{
						perror("Error al enviar");
						exit(1);
					}

					fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", esMayoroMenor);
					fprintf(log, "===================================================================\n");
					continue;
				}

			} while (!next && tries > 0);

			if (tries == 0)
			{
				len = send(sock, "375 FALLO\r\n", sizeof("375 FALLO\r\n"), 0);
				if (len == -1)
				{
					perror("Error al enviar");
					exit(1);
				}
				fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", "375 FALLO\r\n");
				break;
			}
		}
		else
		{
			len = send(sock, SYNTAX_ERROR, sizeof(SYNTAX_ERROR), 0);
			if (len == -1)
			{
				perror("Error al enviar");
				exit(1);
			}
			fprintf(log, "RECEIVED AT %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(clientaddr_in.sin_addr), "TCP", buf);
			fprintf(log, "\n");
			fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", SYNTAX_ERROR);
			fprintf(log, "===================================================================\n");
			continue;
		}
	}

	/* Log a finishing message. */
	fclose(log);

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

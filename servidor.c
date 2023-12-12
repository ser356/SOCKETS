/*
** Fichero: servidor.c
** Autores:
** ser365 (https://github.com/ser356)
** andresblz (https://github.com/andresblz)
*/
#include "socketutils.h"
#define true 1
#define false 0
#include <semaphore.h>

sem_t udpSemaphore;
sem_t tcpSemaphore;
int FIN = 0; /* Para el cierre ordenado */
void finalizar() { FIN = 1; }
int main(int argc, char **argv)
{
	int s_TCP, s_UDP;	/* connected socket descriptor */
	int ls_TCP, ls_UDP; /* listening socket descriptor */

	int br; /* contains the number of bytes read */

	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

	struct sockaddr_in myaddr_in;	  /* for local socket address */
	struct sockaddr_in clientaddr_in; /* for peer socket address */
	socklen_t addrlen;

	fd_set readmask;
	int numfds, s_bigger;

	char buffer[TAM_BUFFER]; /* buffer for packets to be read into */

	struct sigaction vec;

	/* Register SIGALARM */
	vec.sa_handler = (void *)handler;
	vec.sa_flags = 0;
	if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
	{
		perror(" sigaction(SIGALRM)");
		fprintf(stderr, "%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}

	/* Create the listen TCP socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}

	/* Clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
	/* The server should listen on the wildcard address,
	 * rather than its own internet address.  This is
	 * generally good practice for servers, because on
	 * systems which are connected to more than one
	 * network at once will be able to have one server
	 * listening on all networks at once.  Even when the
	 * host is connected to only one network, this is good
	 * practice, because it makes the server program more
	 * portable.
	 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}

	/* Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}

	/* Create the socket UDP. */
	ls_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (ls_UDP == -1)
	{
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}

	/* Bind the server's address to the socket. */
	if (bind(ls_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	/* Now, all the initialization of the server is
	 * complete, and any user errors will have already
	 * been detected.  Now we can fork the daemon and
	 * return to the user.  We need to do a setpgrp
	 * so that the daemon will no longer be associated
	 * with the user's control terminal.  This is done
	 * before the fork, so that the child will not be
	 * a process group leader.  Otherwise, if the child
	 * were to open a terminal, it would become associated
	 * with that terminal as its control terminal.  It is
	 * always best for the parent to do the setpgrp.
	 */
	setpgrp();

	switch (fork())
	{
	case -1: /* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0: /* The child process (daemon) comes here. */

		/* Close stdin and stderr so that they will not
		 * be kept open.  Stdout is assumed to have been
		 * redirected to some logging file, or /dev/null.
		 * From now on, the daemon will not report any
		 * error messages.  This daemon will loop forever,
		 * waiting for connections and forking a child
		 * server to handle each one.
		 */
		fclose(stdin);
		fclose(stderr);

		/* Set SIGCLD to SIG_IGN, in order to prevent
		 * the accumulation of zombies as each child
		 * terminates.  This means the daemon does not
		 * have to make wait calls to clean them up.
		 */
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror(" sigaction(SIGCHLD)");
			fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
			exit(1);
		}

		/* Register SIGTERM to create an orderly completion  */
		vec.sa_handler = (void *)finalizar;
		vec.sa_flags = 0;
		if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1)
		{
			perror(" sigaction(SIGTERM)");
			fprintf(stderr, "%s: unable to register the SIGTERM signal\n", argv[0]);
			exit(1);
		}

		while (!FIN)
		{
			/* Add both sockets to the mask */
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(ls_UDP, &readmask);

			/* Select the socket descriptor that has changed. It leaves a
			   mark in the mask. */
			if (ls_TCP > ls_UDP)
				s_bigger = ls_TCP;
			else
				s_bigger = ls_UDP;

			if ((numfds = select(s_bigger + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
			{
				if (errno == EINTR)
				{
					FIN = true;
					close(ls_TCP);
					close(ls_UDP);
					perror("\nFinalizando el servidor. Senial recibida en select\n");
				}
			}
			else
			{
				/* Check if the selected socket is TCP */
				if (FD_ISSET(ls_TCP, &readmask))
				{
					sleep(1);
					/* Note that addrlen is passed as a pointer
					 * so that the accept call can return the
					 * size of the returned address.
					 */

					/* This call will block until a new
					 * connection arrives.  Then, it will
					 * return the address of the connecting
					 * peer, and a new socket descriptor, s,
					 * for that connection.
					 */
					s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (s_TCP == -1)
						exit(1);

					switch (fork())
					{
					case -1: /* Can't fork, just exit. */
						exit(1);

					case 0: /* Child process comes here. */
						/* Close the listen socket inherited from the daemon. */
						close(ls_TCP);

						// Starts up the server
						serverTCP(s_TCP, clientaddr_in, myaddr_in);
						exit(0);

					default: /* Daemon process comes here. */
							 /* The daemon needs to remember
							  * to close the new abrept socket
							  * after forking the child.  This
							  * prevents the daemon from running
							  * out of file descriptor space.  It
							  * also means that when the server
							  * closes the socket, that it will
							  * allow the socket to be destroyed
							  * since it will be the last close.
							  */
						close(s_TCP);
					}

				} /* End TCP*/

				/* Check if the selected socket is UDP */
				/* Check if the selected socket is UDP */
				/* Check if the selected socket is UDP */
				if (FD_ISSET(ls_UDP, &readmask))
				{

					sleep(5);					
					/* This call will block until a new
					 * request arrives.  Then, it will create
					 * a false "TCP" connection and working the same
					 * as TCP works creating a new socket for that
					 * false connection.
					 */
					br = recvfrom(ls_UDP, buffer, 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (br == -1)
					{
						perror(argv[0]);
						printf("%s: recvfrom error (failed false connection UDP)\n", argv[0]);
						exit(1);
					}

					/* When a new client sends a UDP datagram, his information is stored
					 * in "clientaddr_in", so we can create a false connection by sending messages
					 * manually with this information
					 */
					s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
					if (s_UDP == -1)
					{
						perror(argv[0]);
						printf("%s: unable to create new socket UDP for new client\n", argv[0]);
						exit(1);
					}

					/* Clear and set up address structure for new socket.
					 * Port 0 is specified to get any of the avaible ones, as well as the IP address.
					 */
					memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
					myaddr_in.sin_family = AF_INET;
					myaddr_in.sin_addr.s_addr = INADDR_ANY;
					myaddr_in.sin_port = htons(0);

					/* Bind the server's address to the new socket for the client. */
					if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
					{
						perror(argv[0]);
						printf("%s: unable to bind address new socket UDP for new client\n", argv[0]);
						exit(1);
					}

					/* As well as its done in TCP, a new thread is created for that false connection */
					switch (fork())
					{
					case -1:
						exit(1);

					case 0: /* Child process comes here. */
							/* Child doesnt need the listening socket */
						close(ls_UDP);

						/* Sends a message to the client for him to know the new port for
						 * the false connection
						 */
						if (sendto(s_UDP, " ", 1, 0, (struct sockaddr *)&clientaddr_in, addrlen) == -1)
						{
							perror(argv[0]);
							fprintf(stderr, "%s: unable to send request to \"connect\" \n", argv[0]);
							exit(1);
						}
 

						// Starts up the server
						serverUDP(s_UDP, clientaddr_in, myaddr_in);

						exit(0);

					
					}

				} /* End UDP*/
			}

		} /* End new clients loop */

		/* Close sockets before stopping the server */
		close(ls_TCP);
		close(s_UDP);

		printf("\nFin de programa servidor!\n");

	default: /* Parent process comes here. */
		exit(0);
	}

} // End switch

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
	char *logfile = "peticiones.log";
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
			fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s\n", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "TCP", SYNTAX_ERROR);
			fprintf(log, "===================================================================\n");
			continue;
		}
	}

	/* Log a finishing message. */
	fclose(log);

} // End serverTCP

void serverUDP(int socketUDP, struct sockaddr_in clientaddr_in, struct sockaddr_in myaddr_in)
{

	char hostname[MAXHOST]; /* remote host's name string */

	int status;
	long timevar; /* contains time returned by time() */
	socklen_t addrlen = sizeof(struct sockaddr_in);

	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */

	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);
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

	time(&timevar);
	// printf("[SERV UDP] Startup from %s port %u at %s",
	//	hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

	char *logfile = "peticiones.log";
	FILE *log = openLog(logfile);
	char buf[TAM_BUFFER]; /* This example uses TAM_BUFFER byte messages. */
	int next = 0;
	int len;
	int tries = 5;
	if (log == NULL)
	{
		// THE ONLY ERROR MSG THAT COULDNT BE LOGGED
		fprintf(stderr, "FATAL!: No se pudo abrir el archivo de log!\n");
		fflush(stdout);

		exit(1);
	}

	int numberOfLines;
	char **matrizPreguntas = readArchivoPreguntas("archivopreguntas.txt", &numberOfLines);

	fprintf(log, "===================================================================\n");
	fflush(log);

	fprintf(log, "STARTING LOG AT %s\n", getCurrentTimeStr());
	fflush(log);

	fprintf(log, "NUMERO DE LINEAS LEIDAS: %d\n", numberOfLines);
	fflush(log);

	fprintf(log, "===================================================================\n");
	fflush(log);

	char **matrizRespuestas = readArchivoRespuestas("archivorespuestas.txt");

	if (matrizPreguntas == NULL)
	{

		fprintf(log, "FATAL!: No se pudo leer el archivo de preguntas!\n");
		fflush(log);

		exit(1);
	}
	if (matrizRespuestas == NULL)
	{
		fprintf(log, "FATAL!: No se pudo leer el archivo de respuestas!\n");
		fflush(log);

		exit(1);
	}

	fprintf(log, "SERVER SENDING at %s on PORT %d|%s|%s|%s|%s", getCurrentTimeStr(), ntohs(clientaddr_in.sin_port), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", "220 SERVICIO PREPARADO\r\n");
	fflush(log);
	fprintf(log, "===================================================================\n");
	fflush(log);
	int index;
	char *lineofFileofQuestions;

	while (1)
	{

		recvfrom(socketUDP, buf, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, &addrlen);

		//if (esAdios(buf))
		//{
			//break;
		//}

		if (strcmp(buf, "HOLA\r\n") == 0)
		{
			fprintf(log, "RECEIVED HOLA at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(clientaddr_in.sin_addr), "UDP", buf);
			fflush(log);
			fprintf(log, "===================================================================\n");
			fflush(log);
		}

		if (strcmp(buf, HOLA) == 0 || next == 1)
		{
			next = 0;
			lineofFileofQuestions = getRandomQuestion(matrizPreguntas, &index);

			lineofFileofQuestions[strlen(lineofFileofQuestions) - 1] = '\0';

			char response[TAM_BUFFER] = "250 ";
			sprintf(response, "250 %s#%d\r\n", lineofFileofQuestions, tries);
			len = sendto(socketUDP, response, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, addrlen);

			if (len == -1)
			{
				perror("Error al enviar");
				exit(1);
			}
			fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", response);
			fflush(log);
			fprintf(log, "===================================================================\n");

			char *respuesta = getAnswerFromIndex(index, matrizRespuestas);
			respuesta[strlen(respuesta) - 1] = '\0';
			strcat(respuesta, "\r\n");

			char esMayoroMenor[TAM_BUFFER];

			do
			{
				// Recibir respuesta del cliente
				sleep(1);
				len = recvfrom(socketUDP, buf, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, &addrlen);

				if (len == -1)
				{
					perror("Error al recibir");
					exit(1);
				}
				fprintf(log, "RECEIVED at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(clientaddr_in.sin_addr), "UDP", buf);
				fflush(log);
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
						len = sendto(socketUDP, esMayoroMenor, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, addrlen);
						// dont want to interact with the rest of the code
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", esMayoroMenor);
						fflush(log);

						fprintf(log, "===================================================================\n");
						fflush(log);

						continue;
					}
					if (strcmp(buf, "ADIOS\r\n") == 0)
					{
						len = sendto(socketUDP, ADIOS, sizeof(ADIOS), 0, (struct sockaddr *)&clientaddr_in, addrlen);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						fprintf(log, "CLOSING at %s on PORT %d |%s|%s|%s|%s", getCurrentTimeStr(), ntohs(clientaddr_in.sin_port), hostname, inet_ntoa(clientaddr_in.sin_addr), "UDP", "ADIOS\r\n");
						fflush(log);

						fprintf(log, "===================================================================\n");
						fflush(log);

						break;
					}

					if (number > atoi(respuesta))
					{
						tries--;
						sprintf(esMayoroMenor, "354 %s#%d", MENOR, tries);
						strcat(esMayoroMenor, "\r\n");
						len = sendto(socketUDP, esMayoroMenor, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, addrlen);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", esMayoroMenor);
						fflush(log);

						fprintf(log, "===================================================================\n");
						fflush(log);
					}

					if (number < atoi(respuesta))
					{
						tries--;
						sprintf(esMayoroMenor, "354 %s#%d", MAYOR, tries);
						strcat(esMayoroMenor, "\r\n");
						len = sendto(socketUDP, esMayoroMenor, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, addrlen);
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", esMayoroMenor);
						fflush(log);

						fprintf(log, "===================================================================\n");
						fflush(log);
					}
					if (number == atoi(respuesta))
					{
						len = sendto(socketUDP, ACIERTO, sizeof(ACIERTO), 0, (struct sockaddr *)&clientaddr_in, addrlen);

						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", ACIERTO);
						fflush(log);

						fprintf(log, "===================================================================\n");
						fflush(log);

						next = 1;
					}
				}
				else
				{
					tries--;
					sprintf(esMayoroMenor, "%s#%d\r\n", SYNTAX_ERROR, tries);
					len = sendto(socketUDP, esMayoroMenor, sizeof(esMayoroMenor), 0, (struct sockaddr *)&clientaddr_in, addrlen);
					if (len == -1)
					{
						perror("Error al enviar");
						exit(1);
					}

					fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", esMayoroMenor);
					fflush(log);

					fprintf(log, "===================================================================\n");
					fflush(log);

					continue;
				}

			} while (!next && tries > 0);

			if (tries == 0)
			{
				len = sendto(socketUDP, "375 FALLO\r\n", sizeof("375 FALLO\r\n"), 0, (struct sockaddr *)&clientaddr_in, addrlen);
				if (len == -1)
				{
					perror("Error al enviar");
					exit(1);
				}
				fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", "375 FALLO\r\n");
				fflush(log);

				break;
			}
		}
		else
		{
			len = sendto(socketUDP, SYNTAX_ERROR, sizeof(SYNTAX_ERROR), 0, (struct sockaddr *)&clientaddr_in, addrlen);
			if (len == -1)
			{
				perror("Error al enviar");
				exit(1);
			}
			fprintf(log, "RECEIVED AT %s|%s|%s|%s|%s", getCurrentTimeStr(), hostname, inet_ntoa(clientaddr_in.sin_addr), "UDP", buf);
			fflush(log);

			fprintf(log, "\n");
			fflush(log);

			fprintf(log, "SERVER SENDING at %s|%s|%s|%s|%s\n", getCurrentTimeStr(), hostname, inet_ntoa(myaddr_in.sin_addr), "UDP", SYNTAX_ERROR);
			fflush(log);

			fprintf(log, "===================================================================\n");
			fflush(log);

			continue;
		}
	}

	fclose(log);

} // End serverUDP

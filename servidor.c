/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.
 *
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
#include "socketutils.h"
#define MAX_requestattempts 5

#define PUERTO 17278
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define TAM_BUFFER 516
#define MAXHOST 128

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in);
void errout(char *); /* declare error out routine */

int FIN = 0; /* Para el cierre ordenado */
void finalizar() { FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{
	// seed, must only be called once, at the beginning of the program
	srand(time(NULL));

	int s_TCP, s_UDP; /* connected socket descriptor */
	int ls_TCP;		  /* listen socket descriptor */

	int cc; /* contains the number of bytes read */

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;

	struct sockaddr_in myaddr_in;	  /* for local socket address */
	struct sockaddr_in clientaddr_in; /* for peer socket address */
	socklen_t addrlen;

	fd_set readmask;
	int numfds, s_mayor;

	char buffer[TAM_BUFFER]; /* buffer for packets to be read into */

	struct sigaction vec;

	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
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
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
	{
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		fflush(stdout);

		exit(1);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fflush(stdout);
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

		/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
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
			/* Meter en el conjunto de sockets los sockets UDP y TCP */
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			/*
			Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
			el conjunto de sockets (readmask)
			*/
			if (ls_TCP > s_UDP)
				s_mayor = ls_TCP;
			else
				s_mayor = s_UDP;

			if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
			{
				if (errno == EINTR)
				{
					FIN = 1;
					close(ls_TCP);
					close(s_UDP);
					perror("\nFinalizando el servidor. Se�al recibida en elect\n ");
				}
			}
			else
			{

				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask))
				{
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

					// starting log for all incoming connections
					createLog("log.log");
					s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (s_TCP == -1)
						exit(1);
					switch (fork())
					{
					case -1: /* Can't fork, just exit. */
						exit(1);
					case 0:			   /* Child process comes here. */
						close(ls_TCP); /* Close the listen socket inherited from the daemon. */
						serverTCP(s_TCP, clientaddr_in);
						exit(0);
					default: /* Daemon process comes here. */
							 /* The daemon needs to remember
							  * to close the new accept socket
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
				} /* De TCP*/
				/* Comprobamos si el socket seleccionado es el socket UDP */
				if (FD_ISSET(s_UDP, &readmask))
				{
					/* This call will block until a new
					 * request arrives.  Then, it will
					 * return the address of the client,
					 * and a buffer containing its request.
					 * BUFFERSIZE - 1 bytes are read so that
					 * room is left at the end of the buffer
					 * for a null character.
					 */
					cc = recvfrom(s_UDP, buffer, TAM_BUFFER - 1, 0,
								  (struct sockaddr *)&clientaddr_in, &addrlen);
					if (cc == -1)
					{
						perror(argv[0]);

						printf("%s: recvfrom error\n", argv[0]);
						fflush(stdout);

						exit(1);
					}
					/* Make sure the message received is
					 * null terminated.
					 */
					buffer[cc] = '\0';
					serverUDP(s_UDP, buffer, clientaddr_in);
				}
			}
		} /* Fin del bucle infinito de atenci�n a clientes */
		/* Cerramos los sockets UDP y TCP */

		close(s_TCP);
		fflush(stdout);
		fflush(stdout);
		printf("\nFin de programa servidor!\n");
		fflush(stdout);

	default: /* Parent process comes here. */
		exit(0);
	}
}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int sock, struct sockaddr_in clientaddr_in)
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
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger,
				   sizeof(linger)) == -1)
	{
		errout(hostname);
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

	int numberOfLines;
	char **matrizPreguntas = readArchivoPreguntas("archivopreguntas.txt", &numberOfLines);
	fflush(stdout);
	char *logfile = "peticiones.txt";
	FILE *log = openLog(logfile);
	if (log == NULL)
	{
		fprintf(stderr, "FATAL!: No se pudo abrir el archivo de log!\n");
		fflush(stdout);

		exit(1);
	}

	fprintf(log, "NUMERO DE LINEAS LEIDAS: %d\n", numberOfLines);
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
	fprintf(log, "S:220 SERVICIO PREPARADO\r\n");
	int intentosJuego = 15;
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

			fprintf(log, "C:%s", buf);
		}

		if (strcmp(buf, HOLA) == 0 || next == 1)
		{

			next = 0;
			lineofFileofQuestions = getRandomQuestion(matrizPreguntas, &index);

			lineofFileofQuestions[strlen(lineofFileofQuestions) - 1] = '\0';

			char response[TAM_BUFFER] = "250 ";
			sprintf(response, "250 %s#%d\r\n", lineofFileofQuestions, intentosJuego);
			len = send(sock, response, TAM_BUFFER, 0);
			fprintf(log, "S:%s", response);
			if (len == -1)
			{
				perror("Error al enviar");
				exit(1);
			}

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
				fprintf(log, "C:%s", buf);
				fflush(stdout);
				/*
				LA RESPUESTA VIENE EN FORMATO "RESPUESTA <NUMERO>\r\n"
				POR LO QUE SE DEBE SEPARAR EL NUMERO DEL RESTO DEL STRING
				SINO -> ERROR DE SINTAXIS
				*/
				char evaluar[] = "RESPUESTA ";
				int number;
				int isvalidresponse = sscanf(buf, "%s %d\r\n", evaluar, &number);
				if (strcmp(buf, ADIOS) == 0 || (isvalidresponse == 2))
				{ // Verificar si el cliente envió "ADIOS"
					if (strcmp(buf, "ADIOS\r\n") == 0)
					{
						len = send(sock, ADIOS, sizeof(ADIOS), 0);
						fprintf(log, "S:%s", ADIOS);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
						break;
					}

					if (atoi(buf) > atoi(respuesta))
					{
						intentosJuego--;
						sprintf(esMayoroMenor, "354 %s#%d", MENOR, intentosJuego);
						strcat(esMayoroMenor, "\r\n");
						len = send(sock, esMayoroMenor, TAM_BUFFER, 0);
						fprintf(log, "S:%s", esMayoroMenor);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}
					}
					if (atoi(buf) < atoi(respuesta))
					{
						intentosJuego--;
						sprintf(esMayoroMenor, "354 %s#%d", MAYOR, intentosJuego);
						strcat(esMayoroMenor, "\r\n");
						len = send(sock, esMayoroMenor, TAM_BUFFER, 0);
						fprintf(log, "S:%s", esMayoroMenor);
					}
					if (atoi(buf) == atoi(respuesta))
					{
						len = send(sock, ACIERTO, sizeof(ACIERTO), 0);
						fprintf(log, "S:%s", ACIERTO);
						if (len == -1)
						{
							perror("Error al enviar");
							exit(1);
						}

						next = 1;
					}
				}
				else
				{
					len = send(sock, SYNTAX_ERROR, sizeof(SYNTAX_ERROR), 0);
					fprintf(log, "S:%s", SYNTAX_ERROR);
					if (len == -1)
					{
						perror("Error al enviar");
						exit(1);
					}
					continue;
				}

			} while (!next);
		}
		else
		{
			fprintf(log, "C:%s", buf);
			len = send(sock, SYNTAX_ERROR, sizeof(SYNTAX_ERROR), 0);
			fprintf(log, "S:%s", SYNTAX_ERROR);
			if (len == -1)
			{
				perror("Error al enviar");
				exit(1);
			}
			continue;
		}
	}

	/* Log a finishing message. */
	fclose(log);
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	fprintf(stderr, "Exiting from %s\n", hostname);
	fflush(stdout);
	exit(1);
}

void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in)
{
}

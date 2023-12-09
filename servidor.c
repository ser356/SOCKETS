#include "socketutils.h"
#define true 1
#define false 0
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
				if (FD_ISSET(ls_UDP, &readmask))
				{
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

					default:
						close(s_UDP);
					}

				} /* End UDP*/
			}

		} /* End new clients loop */

		/* Close sockets before stopping the server */
		close(ls_TCP);
		close(ls_UDP);

		printf("\nFin de programa servidor!\n");

	default: /* Parent process comes here. */
		exit(0);
	}

} // End switch

void serverUDP(int socketUDP, struct sockaddr_in clientaddr_in, struct sockaddr_in myaddr_in)
{

	FILE *log = openLog("peticiones.txt");
	char *hostname = "localhost";
	if (log == NULL)
	{
		// THE ONLY ERROR MSG THAT COULDNT BE LOGGED
		fprintf(stderr, "FATAL!: No se pudo abrir el archivo de log!\n");
		fflush(stdout);

		exit(1);
	}
	struct in_addr reqaddr; /* for requested host's address */
	int errcode;
	int len;

	struct addrinfo hints, *res;

	socklen_t addrlen;

	addrlen = sizeof(struct sockaddr_in);
	char buf[TAM_BUFFER];
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* Treat the message as a string containing a hostname. */
	/* Esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
	errcode = getaddrinfo(buf, NULL, &hints, &res);

	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
	}
	else
	{
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);

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

	int index;
	int next = 0;
	char *lineofFileofQuestions;

	printf("Servidor UDP listo para recibir peticiones\n");
}
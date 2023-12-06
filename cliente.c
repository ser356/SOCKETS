#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#define PUERTO 17278
#define TAM_BUFFER 516
#define MAX_ATTEMPTS 5
void clienteTCP(char *program, char *hostname, char *protocol, char *filename);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Uso: %s <host> <protocolo> <archivo>\n", argv[0]);
        exit(1);
    }
    if (strcmp(argv[2], "tcp") != 0)

    {
        fprintf(stderr, "Protocolo %s no soportado\n", argv[2]);
        fprintf(stderr, "Uso: %s <host> <protocolo> <archivo>\n", argv[0]);
        exit(1);
    }

    clienteTCP(argv[0], argv[1], argv[2], argv[3]);

    return 0;
}
void clienteTCP(char *program, char *hostname, char *protocol, char *filename)
{
    int s, len, len1; /* connected socket descriptor */
    struct addrinfo hints, *res;
    long timevar;                   /* contains time returned by time() */
    struct sockaddr_in myaddr_in;   /* for local socket address */
    struct sockaddr_in servaddr_in; /* for server socket address */
    socklen_t addrlen;
    int errcode;
    /* This example uses TAM_BUFFER byte messages. */
    char buf[TAM_BUFFER];
    char *file = malloc(strlen(filename + 1));
    if (file == NULL)
    {
        perror(program);
        fprintf(stderr, "%s: unable to allocate memory\n", program);
        exit(1);
    }
    strncpy(file, filename, strlen(filename) + 1);
    FILE *fp;
    if ((fp = fopen(file, "r")) == NULL)
    {
        perror(program);
        fprintf(stderr, "%s: unable to open file %s\n", program, file);
        exit(1);
    }
    /* Create the socket. */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        perror(program);
        fprintf(stderr, "%s: unable to create socket\n", program);
        exit(1);
    }

    /* clear out address structures */
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

    /* Set up the peer address to which we will connect. */
    servaddr_in.sin_family = AF_INET;

    /* Get the host information for the hostname that the
     * user passed in. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo(hostname, NULL, &hints, &res);
    if (errcode != 0)
    {
        /* Name was not found.  Return a
         * special value signifying the error. */
        fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
                program, hostname);
        exit(1);
    }
    else
    {
        /* Copy address of host */
        servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);

    /* puerto del servidor en orden de red*/
    servaddr_in.sin_port = htons(PUERTO);

    /* Try to connect to the remote server at the address
     * which was just built into peeraddr.
     */
    if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        perror(program);
        fprintf(stderr, "%s: unable to connect to remote\n", program);
        exit(1);
    }
    /* Since the connect call assigns a free address
     * to the local end of this connection, let's use
     * getsockname to see what it assigned.  Note that
     * addrlen needs to be passed in as a pointer,
     * because getsockname returns the actual length
     * of the address.
     */
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
    {
        perror(program);
        fprintf(stderr, "%s: unable to read socket address\n", program);
        exit(1);
    }

    /* Print out a startup message for the user. */
    time(&timevar);
    /* The port number must be converted first to host byte
     * order before printing.  On most hosts, this is not
     * necessary, but the ntohs() call is included here so
     * that this program could easily be ported to a host
     * that does require it.
     */

    printf("Connected to %s on port %u at %s",
           hostname, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));
    fflush(stdout);

    int intentos = 0; // Agrega un contador para los mensajes
    size_t tam;
    memset(buf, 0, TAM_BUFFER);
    recv(s, buf, TAM_BUFFER, 0);
    printf("%s", buf);
    fflush(stdout);

    while (fgets(buf, TAM_BUFFER, fp) != NULL)
    {

        tam = strlen(buf);
        // If the last char is \n, replace it with \0
        if (tam > 0 && buf[tam - 1] == '\n')
        {
            buf[tam - 1] = '\0';
        }
        // Data sending to server
        // MATCHING THE PROTOCOL FORMAT
        // add  \r\n to the end of the message
        strcat(buf, "\r\n");

        len = send(s, buf, TAM_BUFFER, 0);
        if (len == -1)
        {
            perror(program);
            fprintf(stderr, "%s: Imposible enviar\n", program);
            intentos++;
        }
        memset(buf, 0, TAM_BUFFER);
        // Data receiving from server
        len = recv(s, buf, TAM_BUFFER, 0);
        if (len == -1)
        {
            perror(program);
            fprintf(stderr, "%s: Imposible recibir\n", program);
            intentos++;
        }

        printf("S: %s", buf);
        fflush(stdout);

        intentos++;
    }

    

    fclose(fp);
    free(file);

    /* Print message indicating completion of task. */
    sleep(1);
    time(&timevar);
    printf("All done at %s", (char *)ctime(&timevar));
    fflush(stdout);

    close(s);
}

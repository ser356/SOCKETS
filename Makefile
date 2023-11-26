CC = gcc
CFLAGS = -Wall -g

all: servidor cliente

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c

ejecutaOrdenes:
	./lanzaServidor.sh

clean:
	rm -f servidor cliente

lean:
	rm -f *.o

kill:
	killall -9 servidor
	killall -15 servidor
run :
	./servidor && ./cliente localhost tcp dd
CC = gcc
CFLAGS = -Wall -g

all: servidor cliente

socketutils.o: socketutils.c socketutils.h
	$(CC) $(CFLAGS) -c socketutils.c

servidor: servidor.c socketutils.o
	$(CC) $(CFLAGS) -o servidor servidor.c socketutils.o

cliente: cliente.c socketutils.o
	$(CC) $(CFLAGS) -o cliente cliente.c socketutils.o

ejecutaOrdenes:
	./lanzaServidor.sh

clean:
	rm -f servidor cliente socketutils.o

lean:
	rm -f *.o

kill:
	killall -9 servidor
	killall -15 servidor

run :
	./servidor && ./cliente localhost tcp dd
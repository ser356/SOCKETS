CC = gcc
CFLAGS = -Wall -g -m32 -std=gnu11

all: servidor cliente

socketutils.o: socketutils.c socketutils.h
	$(CC) $(CFLAGS) -c socketutils.c -o socketutils.o -m32 -lpthread -lm -lrt

servidor: servidor.c socketutils.o
	$(CC) $(CFLAGS) -o servidor servidor.c socketutils.o -m32 -lpthread -lm -lrt	

cliente: cliente.c socketutils.o
	$(CC) $(CFLAGS) -o cliente cliente.c socketutils.o -m32 -lpthread -lm -lrt	

ejecutaOrdenes:
	./lanzaServidor.sh

clean:
	rm -f servidor cliente socketutils.o

lean:
	rm -f *.o

kill:
	killall -9 servidor
unlog:
	rm -f cliente.log peticiones.log
run :
	./servidor && ./cliente localhost tcp dd

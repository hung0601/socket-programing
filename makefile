CC = gcc

all: client server

debug: CFLAGS += -g
debug: client server

client: client.o 
	${CC} client.o -o client -lpthread

server: server.o linklist.o
	${CC} server.o linklist.o -o server -lpthread
linklist.o: linklist.c
	${CC} -c linklist.c
client.o: client.c
	${CC} -c client.c 

server.o: server.c
	${CC} -c server.c 

clean:
	rm -f *.o *~
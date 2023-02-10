CC = gcc
SDL =`sdl2-config --libs --cflags` -ggdb3 -O0 --std=c99 -Wall -lSDL2_image -lSDL2_ttf -lm
all: client server

debug: CFLAGS += -g
debug: client server

client: client.o linklist.o
	${CC} client.o linklist.o -o client -lpthread -lssl -lcrypto ${SDL}

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
CC=gcc

client : client.o
	$(CC) -o client client.o

client.o : client.c
	$(CC) -g -c client.c

server : server.o
	$(CC) -o server server.o -lpthread -lcrypt

server.o : server.c
	$(CC) -g -c server.c -lpthread -lcrypt
clean:
	rm client client.o server server.o


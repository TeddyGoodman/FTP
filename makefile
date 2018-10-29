all: server.out client.out

clean:
	rm server.out client.out

server.out: server.c utility.c command.c session.c
	gcc -o server.out -Wall -pthread server.c utility.c command.c session.c

client.out: client.c
	gcc -o client.out -Wall client.c

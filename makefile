all: server client

clean:
	rm server client

server: server.c utility.c command.c session.c
	gcc -o server -Wall -pthread server.c utility.c command.c session.c

client: client.c
	gcc -o client -Wall client.c

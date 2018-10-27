all: server.out client.out

clean:
	rm server.out client.out

server.out: server.c
	gcc -o server.out -Wall server.c

client.out: client.c
	gcc -o client.out -Wall client.c

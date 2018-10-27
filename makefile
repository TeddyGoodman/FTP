all: server.out

clean:
	rm server.out

server.out: server.c
	gcc -o server.out -Wall server.c

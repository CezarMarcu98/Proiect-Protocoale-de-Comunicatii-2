all: server subscriber

subscriber: client.c
	gcc client.c -lm -o subscriber

server: server.c
	gcc server.c -lm -o server

clean: 
	rm -rf server subscriber

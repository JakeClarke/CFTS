all: client server

client: ./client/client.c
	clang -o client.out ./client/client.c

server: ./server/server.c
	clang -o server.out ./server/server.c

clean:
	rm client.out
	rm server.out
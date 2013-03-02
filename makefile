all: client server

client: ./client/client.c
	clang -o aos-client ./client/client.c

server: ./server/server.c
	clang -o aos-server ./server/server.c

clean:
	rm aos-client
	rm aos-server
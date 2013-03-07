flags = -g
all: aos-client aos-server

aos-client: ./client/client.c
	clang $(flags) -o aos-client ./client/client.c

aos-server: ./server/server.c
	clang $(flags) -o aos-server ./server/server.c

clean:
	rm aos-client
	rm aos-server
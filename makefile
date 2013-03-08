flags = -g
all: aos-client aos-server

aos-client: client.c
	clang $(flags) -pthread -o aos-client client.c

aos-server: server.c
	clang $(flags) -o aos-server server.c

clean:
	rm aos-client
	rm aos-server
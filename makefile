flags = -g
all: aos-client aos-server

aos-client: client.c enc.h messages.h
	clang $(flags) -pthread -o aos-client client.c

aos-server: server.c enc.h messages.h config.h
	clang $(flags) -o aos-server server.c

clean:
	rm aos-client
	rm aos-server
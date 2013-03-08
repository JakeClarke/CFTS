#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "messages.h"

#define SIZE sizeof(struct sockaddr_in)
#define CLIENTBUFF_SIZE 256

int sockfd, clientsockfd;

int main(int argc, char *argv[]) 
{
	pid_t pid;
	char * configLoc = "~/.AOS.config";
	char * wkDir = "./";

	struct sockaddr_in server;
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(4321);

	printf("AOS server.\n");
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("AOS-server", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
	syslog(LOG_INFO, "Server started!");

	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "-c") == 0)
		{
			syslog(LOG_DEBUG, "Detected config flag");
			if(i + 1 < argc)
				configLoc = argv[i + 1];
		}
	}

	printf("Loading config: %s\n", configLoc);

	/* TODO config loading */

	/* Change the working dir. */
	if(chdir(wkDir) != 0) {
		printf("Failed to change to working directory: %s\n", wkDir);
		exit(EXIT_FAILURE);
	}
	else {
		printf("Working dir: %s\n", wkDir);
	}

	/* Bind to socket and listen. */
	printf("Opening socket on port: %i\n", ntohs(server.sin_port));

	if(bind(sockfd, (struct sockaddr *)&server, SIZE) ==- 1) {
		printf("Server failed to bind!\n");
		exit(EXIT_FAILURE);
	}


	printf("Forking....\n");
	pid = fork();
	if(pid < 0) {
		syslog(LOG_ERR, "%s\n", perror);
		exit(EXIT_FAILURE);
	}

	if(pid > 0) /* get rid of the parent. */
		exit(EXIT_SUCCESS);

	syslog(LOG_INFO, "Server forked!");

	/* TODO change session */

	if(listen(sockfd,5) == -1) {
		syslog(LOG_ERR, "Server failed to listen!");
		exit(EXIT_FAILURE);
	}

	syslog(LOG_DEBUG, "Socket successfully bind and listening");

	struct sockaddr addr;
	socklen_t addrlen;
	char clientAdd[INET_ADDRSTRLEN];

	for(;;) {

		clientsockfd = accept(sockfd, &addr, &addrlen);
		inet_ntop(addr.sa_family, &addr.sa_data, &clientAdd[0], addrlen);
		syslog(LOG_INFO, "Client Connected! %s", clientAdd);
		if(fork() == 0) {
			CMD_T clientReq = -1;
			char clientBuff[CLIENTBUFF_SIZE] = {0};

			while(recv(clientsockfd, &clientReq, sizeof(CMD_T), 0) > 0) {
				syslog(LOG_DEBUG, "Client request: %i", clientReq);
				if (clientReq == CMD_BYE)
				{
					syslog(LOG_INFO, "Client disconnected!");
					exit(EXIT_SUCCESS);
				}
				else if(clientReq == CMD_SHUTDOWN) {
					syslog(LOG_INFO, "Shutdown recieved!");
					kill(0, SIGTERM);
				}
				else {
					syslog(LOG_ERR, "Unrecognised client request: %i", clientReq);
				}
			}

		}

		close(clientsockfd);

	}
}
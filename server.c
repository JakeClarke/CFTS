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
#include <fcntl.h>
#include "messages.h"

#define SIZE sizeof(struct sockaddr_in)
#define CLIENTBUFF_SIZE 1024

int sockfd, clientsockfd;

void sendFile(int,char*);

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

	printf("AOS-server.\n");
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("AOS-server", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

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
		syslog(LOG_CRIT, "Failed to fork: %s\n", perror);
		exit(EXIT_FAILURE);
	}

	if(pid > 0) /* get rid of the parent. */
		exit(EXIT_SUCCESS);

	if(setsid() == -1) {
		syslog(LOG_CRIT, "Failed to set session id!\n");
		exit(EXIT_FAILURE);
	}

	syslog(LOG_INFO, "Server forked!");

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
				else if(clientReq == CMD_GET) {
					syslog(LOG_DEBUG, "Client file request recieved.");
					// recieve the filename.
					int length = recv(clientsockfd, &clientBuff[0], sizeof(clientBuff), 0);
					syslog(LOG_DEBUG, "recieved: %s - %i", clientBuff, length);
					// copy it for later.
					char fileName[length + 1];
					fileName[length] = '\0';
					memcpy(&fileName[0], &clientBuff[0], length);

					sendFile(clientsockfd, &fileName[0]);

				}
				else {
					syslog(LOG_ERR, "Unrecognised client request: %i", clientReq);
				}

				memset(&clientBuff[0],0, sizeof(clientBuff));
			}

		}

		close(clientsockfd);

	}
}

void sendFile(int socket, char * file) {
	syslog(LOG_INFO, "Client requested file: %s", file);
	char clientBuff[CLIENTBUFF_SIZE] = {0};

	int fileFD = open(file, O_RDONLY);
	if(fileFD > -1) {
		long fileLength = lseek(fileFD, 0, SEEK_END);
		syslog(LOG_DEBUG, "File length: %ld", fileLength);

		lseek(fileFD, 0, SEEK_SET);

		send(socket, &SERVE_FILE, sizeof(SERVE_FILE), 0);
		int fileNameLength = (strlen(file) + 1) * sizeof(char);
		syslog(LOG_DEBUG, "File name length: %i", fileNameLength);
		send(socket, &fileNameLength, sizeof(fileNameLength), 0);
		send(socket, file, fileNameLength * sizeof(char), 0);
		send(socket, &fileLength, sizeof(fileLength), 0);

		// wait for the client to ready.
		CMD_T clientConf;

		recv(socket, &clientConf, sizeof(clientConf), 0);

		if(clientConf != SERVE_GET_BEGIN) {
			syslog(LOG_ERR, "Client refused download!");
			return;
		}
			

		syslog(LOG_DEBUG, "Sending!");

		long currentPos = 0;

		while(currentPos < fileLength) {
			int size = read(fileFD, &clientBuff[0], sizeof(clientBuff));

			if(size == -1) {
				syslog(LOG_CRIT, "Could not read the file.");
				exit(EXIT_FAILURE);
			}

			if(send(socket, &clientBuff[0], size, 0) == -1)
			{
				syslog(LOG_CRIT, "Connection interupted!");
				exit(EXIT_FAILURE);
			}

			currentPos += size;
		}

		close(fileFD);
		syslog(LOG_INFO, "Complete!");
	}
	else {
		syslog(LOG_ERR, "File could not be opened");
		send(socket, &SERVE_GET_ERROR_NOTFOUND, sizeof(SERVE_GET_ERROR_NOTFOUND), 0);
	}
}
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include "messages.h"

#define SIZE sizeof(struct sockaddr_in)
#define RECVBUFF_SIZE 1024

void *recvD(void *);

pthread_t tid;
int sockfd;

int main() 
{
	printf("AOS - client.\n");

	
	char c,rc;
	struct sockaddr_in toserver;

	toserver.sin_family=AF_INET;
	toserver.sin_addr.s_addr = inet_addr("127.0.0.1");
	toserver.sin_port=htons(4321);
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(connect(sockfd, (struct sockaddr *)&toserver, SIZE) == 0) {
		printf("Connected!\n");
	}
	else {
		printf("Failed to connect!\n");
		exit(EXIT_FAILURE);
	}
	// spawn recv thread.
	if(pthread_create(&tid, NULL, &recvD, NULL) != 0) {
		printf("Failed to spawn thread\n");
		exit(EXIT_FAILURE);
	}


	char inBuff[256];
	for(;;) {
		fgets(inBuff, sizeof(inBuff), stdin);
		inBuff[strlen(inBuff) - 1] = '\0'; // get rid of the new line.
		
		if(strcmp(inBuff, "bye") == 0) {
			send(sockfd, &CMD_BYE, sizeof(CMD_GET), 0);
			exit(EXIT_SUCCESS);
		}
		else if(strcmp(inBuff, "shutdown") == 0) {
			send(sockfd, &CMD_SHUTDOWN, sizeof(CMD_SHUTDOWN), 0);
			exit(EXIT_SUCCESS);
		}
		else {
			printf("Invalid input!\n");
		}
	}

	printf("Shutting down!\n");
}

void *recvD(void * args) {
	printf("Created recv thread!\n");
	char recvBuff[RECVBUFF_SIZE];
	for(;;) {
		while(recv(sockfd, &recvBuff, RECVBUFF_SIZE, 0) > 0) {
			if(strcmp(recvBuff, "BYE") == 0) {
				printf("Server closed connection!\n");
				return 0;
			}
			if(strncmp(recvBuff, "FILE", strlen("FILE"))) {
				char * pch;
				strtok(recvBuff, " ");
				pch = strtok(NULL, " ");
				long fileLength = atol(pch);
				long remaining = fileLength;
				pch = strtok(NULL, " ");
				char fileName[sizeof(pch)];
				memcpy(&fileName, &pch, sizeof(pch));

				printf("Recieving file: %s\n", fileName);

				int fileFD = creat(fileName, S_IRWXU);

				while (remaining > 0) {
					int in = recv(sockfd, &recvBuff, RECVBUFF_SIZE, 0);
					write(fileFD, &recvBuff, in);
					remaining -= in;
				}
				close(fileFD);
				printf("File download complete!");

			}
			else {
				printf("Unrecognised server message: %s\n", recvBuff);
			}
		}
	}
}
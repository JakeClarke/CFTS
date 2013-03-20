#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "messages.h"

#define SIZE sizeof(struct sockaddr_in)
#define RECVBUFF_SIZE 1024

void *recvD(void *);
void printBuff(char *, int);

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
		else if(strncmp(inBuff, "get", 3) == 0) {
			if(strlen(inBuff) - 4 > 0) {
				printf("%s\n", &inBuff[4]);
				send(sockfd, &CMD_GET, sizeof(CMD_GET), 0);
				send(sockfd, &inBuff[4], strlen(&inBuff[4]) * sizeof(char), 0);
			}
			else {
				printf("File name required!\n");
			}
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
	CMD_T servMsg = -1;
	for(;;) {
		while(recv(sockfd, &servMsg, sizeof(servMsg), 0) > 0) {
			if(servMsg == SERVE_BYE) {
				printf("Server closed connection!\n");
				exit(EXIT_SUCCESS);
			}
			else if(servMsg == SERVE_FILE) {
				printf("Incoming file!\n");
				int length;
				recv(sockfd, &length, sizeof(length), 0);
				char fileName[length];
				recv(sockfd, &fileName[0], length * sizeof(char), 0);
				
				//memcpy(&fileName, &recvBuff, length);
				//printBuff(&recvBuff[0], length);
				printf("Recieving file: %s\n", fileName);

				long fileLength;
				recv(sockfd, &fileLength, sizeof(fileLength), 0);
				printf("Size file: %ld\n", fileLength);
				long remaining = fileLength;

				int fileFD = creat(fileName, S_IRWXU);

				if(fileFD > 0) {

					send(sockfd, &SERVE_GET_BEGIN, sizeof(SERVE_GET_BEGIN), 0);
					while (remaining > 0) {
						int in = recv(sockfd, &recvBuff, RECVBUFF_SIZE, 0);
						write(fileFD, &recvBuff, in);
						remaining -= in;
						printf("%i\n", in);
					}
					close(fileFD);
					printf("File download complete!\n");
				}
				else {
					send(sockfd, &SERVE_GET_ERROR_CANNOTCREATE, sizeof(SERVE_GET_BEGIN), 0);
					printf("Failed to create file!\n");
					exit(EXIT_FAILURE);
				}
			}
			else if(servMsg == SERVE_GET_ERROR_NOTFOUND) {
				printf("File not found on server!\n");
			}
			else {
				printf("Unrecognised server message: %i\n", servMsg);
			}
		}
	}
}

void printBuff(char * buff, int length) {
	printf("buff dump (size: %i):\n", length);
	for (int i = 0; i < length; ++i)
	{
		printf("%c", buff[i]);
	}
	printf("\n");
}
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "messages.h"
#include "enc.h"

#define SIZE sizeof(struct sockaddr_in)
#define RECVBUFF_SIZE 1024
#define FILEBUFF_SIZE 1024

void *recvD(void *);
void sendFile(int, int, char *);
void printBuff(char *, int);
void login(void);
void printcmd(void);

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
	if(sockfd == -1) {
		printf("Failed to create socket!\n");
		exit(EXIT_FAILURE);
	}

	if(connect(sockfd, (struct sockaddr *)&toserver, SIZE) == 0) {
		printf("Connected!\n");
	}
	else {
		printf("Failed to connect!\n");
		exit(EXIT_FAILURE);
	}
	// login
	login();

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
			esend(sockfd, &CMD_BYE, sizeof(CMD_GET), 0);
			exit(EXIT_SUCCESS);
		}
		else if(strcmp(inBuff, "shutdown") == 0) {
			esend(sockfd, &CMD_SHUTDOWN, sizeof(CMD_SHUTDOWN), 0);
			exit(EXIT_SUCCESS);
		}
		else if(strncmp(inBuff, "get", 3) == 0) {
			if(strlen(inBuff) - 4 > 0) {
				printf("%s\n", &inBuff[4]);
				esend(sockfd, &CMD_GET, sizeof(CMD_GET), 0);
				esend(sockfd, &inBuff[4], strlen(&inBuff[4]) * sizeof(char), 0);
			}
			else {
				printf("File name required!\n");
			}
		}
		else if (strncmp(inBuff, "put", 3) == 0) {
			if(strlen(inBuff) - 4 > 0) {
				printf("%s\n", &inBuff[4]);
				// open the file before sending the request.
				int fileFD = open(&inBuff[4], O_RDONLY);
				if (fileFD > 0) {
					sendFile(sockfd, fileFD, &inBuff[4]);

					close(fileFD);
				}
				else {
					printf("Could not open file\n");
				}
			}
			else {
				printf("File name required!\n");
			}
		}
		else if (strncmp(inBuff, "cd", 2) == 0) {
			if(strlen(inBuff) - 3 > 0) {
				size_t newWDLength = strlen(&inBuff[3]);
				esend(sockfd, &CMD_CD, sizeof(CMD_CD), 0);
				esend(sockfd, &newWDLength, sizeof(newWDLength), 0);
				esend(sockfd, &inBuff[3], newWDLength * sizeof(char), 0);

			}
		}
		else if (strncmp(inBuff, "exec", 2) == 0) {
			if(strlen(inBuff) - 5 > 0) {
				size_t cmdLength = strlen(&inBuff[5]);
				esend(sockfd, &CMD_EXEC, sizeof(CMD_EXEC), 0);
				esend(sockfd, &cmdLength, sizeof(cmdLength), 0);
				esend(sockfd, &inBuff[5], cmdLength * sizeof(char), 0);
			}
		}
		else if (strncmp(inBuff, "help", 3) == 0) {
			printf("Supported commands:\nget - get a file.\nput - put a file.\ncd - Change directory.\nbye - logout.\nshutdown - shutdown the server.\n");
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
		while(drecv(sockfd, &servMsg, sizeof(servMsg), 0) > 0) {
			if(servMsg == SERVE_BYE) {
				printf("Server closed connection!\n");
				exit(EXIT_SUCCESS);
			}
			else if(servMsg == SERVE_FILE) {
				printf("Incoming file!\n");
				int length;
				drecv(sockfd, &length, sizeof(length), 0);
				char * fileName = (char *)malloc(length);
				drecv(sockfd, fileName, length, 0);
				
				//memcpy(&fileName, &recvBuff, length);
				//printBuff(&recvBuff[0], length);
				printf("Recieving filename length: %i\n", length);
				printf("Recieving file: %s\n", fileName);

				long fileLength;
				drecv(sockfd, &fileLength, sizeof(fileLength), 0);
				printf("Size file: %ld\n", fileLength);
				long remaining = fileLength;

				int fileFD = creat(fileName, S_IRWXU);
				free(fileName);

				if(fileFD > 0) {

					esend(sockfd, &SERVE_GET_BEGIN, sizeof(SERVE_GET_BEGIN), 0);
					while (remaining > 0) {
						int in = drecv(sockfd, &recvBuff, RECVBUFF_SIZE, 0);
						write(fileFD, &recvBuff, in);
						remaining -= in;
						printf("Progress: %ld/%lu\n", (fileLength - remaining), fileLength);
					}
					close(fileFD);
					printf("File download complete!\n");
				}
				else {
					esend(sockfd, &SERVE_GET_ERROR_CANNOTCREATE, sizeof(SERVE_GET_BEGIN), 0);
					printf("Failed to create file!\n");
					exit(EXIT_FAILURE);
				}
			}
			else if(servMsg == SERVE_EXEC_BEGIN) {
				printcmd();
			}
			else if(servMsg == SERVE_GET_ERROR_NOTFOUND) {
				printf("File not found on server!\n");
			}
			else if(servMsg == SERVE_CD_SUCCESS) {
				printf("Working directory changed sucessfully.\n");
			}
			else if(servMsg == SERVE_CD_FAILED) {
				printf("Working directory changed failed.\n");
			}
			else {
				printf("Unrecognised server message: %i\n", servMsg);
			}
		}
	}
}

void sendFile(int sockFD, int fileFD, char * fileName) {

	const long fileLength = lseek(fileFD, 0, SEEK_END);
	if(fileLength == -1 || lseek(fileFD, 0, SEEK_SET) != 0) {
		printf("File seek failed\n");
		return;
	}

	esend(sockFD, &CMD_PUT, sizeof(CMD_PUT), 0);
	size_t fileNameLen = strlen(fileName);
	esend(sockfd, &fileNameLen, sizeof(fileNameLen), 0);
	esend(sockFD, fileName, fileNameLen * sizeof(char), 0);
	esend(sockFD, &fileLength, sizeof(fileLength), 0);

	long remaining = fileLength;

	char fileBuff[FILEBUFF_SIZE];

	while(remaining > 0) {
		int size = read(fileFD, &fileBuff[0], sizeof(fileBuff));
		
		if(size == -1) {
			printf("File read error!\n");
			exit(EXIT_FAILURE);
		}

		if(esend(sockfd, &fileBuff[0], size, 0) == -1) {
			printf("Data send error!\n");
			exit(EXIT_FAILURE);
		}
		remaining -= size;
		
		printf("Progress: %ld/%ld\n", fileLength - remaining, fileLength);
	}

	printf("Put file operation complete!\n");
}

void printBuff(char * buff, int length) {
	printf("buff dump (size: %i):\n", length);
	for (int i = 0; i < length; ++i)
	{
		printf("%c", buff[i]);
	}
	printf("\n");
}

void login(void) {
	char userBuff[256];
	char passBuff[256];
	for (;;)
	{
		printf("Enter username:\n");
		fgets(userBuff, sizeof(userBuff), stdin);
		printf("Enter password:\n");
		fgets(passBuff, sizeof(passBuff), stdin);
		userBuff[strlen(userBuff) - 1] = '\0';
		passBuff[strlen(passBuff) - 1] = '\0';

		size_t len = strlen(userBuff);
		esend(sockfd, &len, sizeof(len), 0);
		esend(sockfd, &userBuff[0], len * sizeof(char), 0);
		len = strlen(passBuff);
		esend(sockfd, &len, sizeof(len), 0);
		esend(sockfd, &passBuff[0], len * sizeof(char), 0);

		CMD_T res = -1;
		if (drecv(sockfd, &res, sizeof(res), 0) > 0 ) {
			switch(res) {
				case LOGIN_SUCCESS:
				printf("Login sucessful.\n");
				return;
				case LOGIN_FAIL:
				printf("Login failed.\n");
				break;
				case SERVE_BYE:
				printf("Too many login attempts, disconnected!\n");
				exit(EXIT_FAILURE);
				break;
				default:
				printf("Unrecognised reply\n");
				break;
			}
		}
		else {
			printf("Socket error!\n");
			exit(EXIT_FAILURE);
		}
	}
}

void printcmd(void) {
	printf("Start of exec output. \n");
	char buff[256];
	int size = 0;
	while((size = drecv(sockfd, &buff[0], sizeof(buff), 0)) > 0 ) {
		if(buff[size - 1] == SERVE_EXEC_END) {
			write(1, &buff[0], size -1);
			fflush(stdout);
			printf("End of exec output. \n");
			return;
		}
		else {
			write(1, &buff[0], size);
		}
	}
}

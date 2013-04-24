#define SERVER
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "messages.h"
#include "enc.h"
#include "config.h"

#define SIZE sizeof(struct sockaddr_in)
#define CLIENTBUFF_SIZE 1024
#define CLIENT_RECV_BUFF_SIZE 1024
#define MAX_LOGIN_ATTEMPTS 3
#define MAX_BIND_ATTEMPTS 3
#define BIND_WAIT_LENGTH 10

int sockfd = 0, clientsockfd = 0;

char * configLoc = "AOS.config";
char blockTerm = 0;

void sendFile(int,char*);
void recvFile(int,char*);
void login(int);
void execCommand(int,char*);
void termHandler(int);

int main(int argc, char *argv[]) 
{
	pid_t pid = 0, pgid = 0;
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
	readConfig(&configLoc[0]);

	struct sockaddr_in server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(port);


	/* Change the working dir. */
	if(chdir(wkDir) == -1) {
		printf("Failed to change to working directory: %s\n", wkDir);
		printf("Error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else {
		printf("Working dir: %s\n", wkDir);
	}
	
	int t = 1;
	// set so that the server can rebind when restarting.
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));
	

	/* Bind to socket and listen. */
	printf("Opening socket on port: %i\n", ntohs(server.sin_port));

	int tries = 0;
	while(bind(sockfd, (struct sockaddr *)&server, SIZE) < 0) {
		printf("%i: Server failed to bind!\n", tries + 1);
		printf("Error: %s\n", strerror(errno));
		++tries;
		if (tries >= MAX_BIND_ATTEMPTS)
		{
			printf("Max attempts hit!\n");
			exit(EXIT_FAILURE);
		}
		printf("Retrying...\n");
		sleep(BIND_WAIT_LENGTH);
	}

	printf("Forking %i time(s)...\n", numberOfChilren);
	pid = fork();
	if(pid > 0) /* get rid of the parent. */
		exit(EXIT_SUCCESS);

	if(setsid() == -1) {
		syslog(LOG_ERR, "Failed to set session id!\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGTERM, termHandler);

	for(int forkNum = 1; forkNum < numberOfChilren; forkNum++) 
	{
		pid = fork();
		if(pid < 0) {
			syslog(LOG_CRIT, "Failed to fork: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if(pgid == 0 && pid != 0) {
			pgid = pid;
		}

		if(pid == 0)
			break;
	}

	syslog(LOG_INFO, "Server forked!");

	if(listen(sockfd,5) == -1) {
		syslog(LOG_ERR, "Server failed to listen!");
		exit(EXIT_FAILURE);
	}

	syslog(LOG_DEBUG, "Socket successfully bind and listening");

	for(;;) {

		struct sockaddr addr;
		socklen_t addrlen = INET_ADDRSTRLEN;
		char clientAdd[INET_ADDRSTRLEN];

		clientsockfd = accept(sockfd, &addr, &addrlen);

		inet_ntop(addr.sa_family, &addr.sa_data, &clientAdd[0], addrlen);

		if(fork() == 0) {
			if(clientsockfd < 0) {
				syslog(LOG_CRIT, "Socket failed to accept connection! Error: %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			else {
				syslog(LOG_INFO, "Client Connected! %s", clientAdd);
			}
			close(sockfd); // don't actually need this any more, bye.
			sockfd = 0; // mark it as closed.

			login(clientsockfd);
			CMD_T clientReq = -1;
			char clientBuff[CLIENTBUFF_SIZE] = {0};

			while(drecv(clientsockfd, &clientReq, sizeof(CMD_T), 0) > 0) {
				syslog(LOG_DEBUG, "Client request: %i", clientReq);
				if (clientReq == CMD_BYE)
				{
					syslog(LOG_INFO, "Client disconnected!");
					exit(EXIT_SUCCESS);
				}
				else if(clientReq == CMD_SHUTDOWN) {
					syslog(LOG_INFO, "Shutdown recieved!");
					if(currAccess && ACC_SHUTDOWN != ACC_SHUTDOWN) {
						syslog(LOG_ERR, "Client does not have the required permission");
						esend(clientsockfd, &ACCESS_DENIED, sizeof(ACCESS_DENIED), 0);
					}
					else {
						kill(0, SIGTERM);
					}
				}
				else if(clientReq == CMD_GET) {
					syslog(LOG_INFO, "Client file request recieved.");
					// recieve the filename.
					int length = drecv(clientsockfd, &clientBuff[0], sizeof(clientBuff), 0);
					syslog(LOG_DEBUG, "recieved: %s - %i", clientBuff, length);
					// copy it for later.
					char fileName[length + 1];
					fileName[length] = '\0';
					memcpy(&fileName[0], &clientBuff[0], length * sizeof(char));

					sendFile(clientsockfd, &fileName[0]);

				}
				else if (clientReq == CMD_PUT)
				{
					syslog(LOG_INFO, "Client file upload request recieved.");
					size_t fileNameLength;
					drecv(clientsockfd, &fileNameLength, sizeof(fileNameLength), 0);
					char fileName[fileNameLength + 1];

					// copy it for later.
					drecv(clientsockfd, &fileName[0], fileNameLength, 0);
					fileName[fileNameLength] = '\0';

					recvFile(clientsockfd, &fileName[0]);
				}
				else if (clientReq == CMD_CD) {
					size_t newWDLength;
					drecv(clientsockfd, &newWDLength, sizeof(newWDLength), 0);
					if(wdChanged)
						free(wkDir);
					wkDir = malloc((newWDLength + 1) * sizeof(char));
					wdChanged = 1;

					drecv(clientsockfd, &wkDir[0], newWDLength, 0);
					wkDir[newWDLength] = '\0';

					if(chdir(wkDir) == 0) {
						esend(clientsockfd, &SERVE_CD_SUCCESS, sizeof(SERVE_CD_SUCCESS), 0);
						syslog(LOG_INFO, "Successfully set working dir: %s", wkDir);
					}
					else {
						esend(clientsockfd, &SERVE_CD_FAILED, sizeof(SERVE_CD_FAILED), 0);
						syslog(LOG_ERR, "Failed to set working dir: %s", wkDir);
					}
				}
				else if (clientReq == CMD_EXEC) {
					syslog(LOG_INFO, "Client exec request recieved.");
					size_t exec_CMD_length;
					drecv(clientsockfd, &exec_CMD_length, sizeof(exec_CMD_length), 0);
					char cmd[exec_CMD_length + 1];
					drecv(clientsockfd, &cmd[0], exec_CMD_length * sizeof(char), 0);
					cmd[exec_CMD_length] = '\0';
					execCommand(clientsockfd, &cmd[0]);
				}
				else if(clientReq == CMD_RESTART) {
					syslog(LOG_INFO, "Restart recieved!");
					if(currAccess && ACC_SHUTDOWN != ACC_SHUTDOWN) {
						syslog(LOG_ERR, "Client does not have the required permission");
						esend(clientsockfd, &ACCESS_DENIED, sizeof(ACCESS_DENIED), 0);
					}
					else {
						esend(clientsockfd, &SERVE_BYE, sizeof(SERVE_BYE), 0);
						shutdown(clientsockfd, 3);
						blockTerm = 1;
						kill(0, SIGTERM);
						if (execv(argv[0], argv)) {
							syslog(LOG_ERR, "Could not restart");
        					exit(EXIT_FAILURE);
   						}
					}

				}
				else {
					syslog(LOG_ERR, "Unrecognised client request: %i", clientReq);
				}

				memset(&clientBuff[0],0, sizeof(clientBuff));
			}

		}
		else {
			// clear children.
			int chPid = waitpid(0, NULL, WNOHANG);
			while(chPid > 0)
			{
				syslog(LOG_DEBUG, "Cleared child process: %i", chPid);
				chPid = waitpid(0, NULL, WNOHANG);
			}
		}

		close(clientsockfd);
		clientsockfd = 0;

	}
}

void recvFile(int socket, char * file) {
	int fileFD = creat(file, S_IRWXU);

	if(fileFD > 0) {
		long fileLength;
		drecv(socket, &fileLength, sizeof(fileLength), 0);
		syslog(LOG_INFO, "Incoming file: %s, Size: %ld", file, fileLength);
		char recvBuff[CLIENT_RECV_BUFF_SIZE];
		while (fileLength > 0) {
			int in = drecv(socket, &recvBuff[0], sizeof(char) * CLIENT_RECV_BUFF_SIZE, 0);

			if(in <= 0) {
				syslog(LOG_CRIT, "Failed to recieve data!");
				exit(EXIT_FAILURE);
			}
			write(fileFD, &recvBuff, in);

			fileLength -= in;
		}

		syslog(LOG_INFO, "File transfer complete!");
		close(fileFD);
	}
	else {
		syslog(LOG_ERR, "Failed to create file: %s", file);
		exit(EXIT_FAILURE);
	}
}

void sendFile(int socket, char * file) {
	syslog(LOG_INFO, "Client requested file: %s", file);
	if(currAccess && ACC_READ != ACC_READ) {
		syslog(LOG_ERR, "Client does not have the required permission");
		esend(socket, &ACCESS_DENIED, sizeof(ACCESS_DENIED), 0);
		return;
	}

	char clientBuff[CLIENTBUFF_SIZE] = {0};

	int fileFD = open(file, O_RDONLY);
	if(fileFD > -1) {
		long fileLength = lseek(fileFD, 0, SEEK_END); // get the file length
		syslog(LOG_DEBUG, "File length: %ld", fileLength);

		lseek(fileFD, 0, SEEK_SET); // go back the start of our file.

		esend(socket, &SERVE_FILE, sizeof(SERVE_FILE), 0); // tell the client that we are about send a file.
		int fileNameLength = (strlen(file) + 1) * sizeof(char); // send the file name plus the null terminator.
		syslog(LOG_DEBUG, "File name length: %i", fileNameLength);
		// after collecting all the details for file download send it to the client.
		esend(socket, &fileNameLength, sizeof(fileNameLength), 0);
		esend(socket, file, fileNameLength * sizeof(char), 0);
		esend(socket, &fileLength, sizeof(fileLength), 0);

		// wait for the client to ready.
		CMD_T clientConf;

		drecv(socket, &clientConf, sizeof(clientConf), 0);

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
				// need to so something a little bit more graceful then just dumping out. The client will have no idea what happened to the server at this point.
				exit(EXIT_FAILURE);
			}

			if(esend(socket, &clientBuff[0], size, 0) == -1)
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
		esend(socket, &SERVE_GET_ERROR_NOTFOUND, sizeof(SERVE_GET_ERROR_NOTFOUND), 0);
	}
}

void login(int socket) {
	syslog(LOG_DEBUG, "%i", socket);
	syslog(LOG_DEBUG, "Client login sequence started!");
	for (int i = 0; i < MAX_LOGIN_ATTEMPTS; ++i)
	{
		size_t len = -1;
		drecv(socket, &len, sizeof(len), 0);
		syslog(LOG_DEBUG, "len: %u", len);
		char userBuff[len + 1];
		drecv(socket, &userBuff[0], len * sizeof(char), 0);
		userBuff[len] = '\0';
		syslog(LOG_DEBUG, "%s uname - %u len.", userBuff, len);

		drecv(socket, &len, sizeof(len), 0);
		syslog(LOG_DEBUG, "len: %u", len);
		char passBuff[len + 1];
		drecv(socket, &passBuff[0], len * sizeof(char), 0);
		passBuff[len] = '\0';
		syslog(LOG_DEBUG, "%s pass - %u len.", passBuff, len);

		if(authUser(&userBuff[0], &passBuff[0])) {
			syslog(LOG_INFO, "%s logged in.", userBuff);
			esend(socket, &LOGIN_SUCCESS, sizeof(LOGIN_SUCCESS), 0);
			return;
		}

		esend(socket, &LOGIN_FAIL, sizeof(LOGIN_FAIL), 0);
	}

	syslog(LOG_ERR, "Max number of login attempts reached, shutting down.");
	esend(socket, &SERVE_BYE, sizeof(SERVE_BYE), 0);
	exit(EXIT_FAILURE);
}

void execCommand(int socket, char* cmd) {
	if(currAccess && ACC_EXEC != ACC_EXEC) {
		syslog(LOG_ERR, "Client does not have the required permission to execute commands.");
		esend(socket, &ACCESS_DENIED, sizeof(ACCESS_DENIED), 0);
		return;
	}

	syslog(LOG_INFO, "Executing command: %s.", cmd);

	int out[2];
	pipe(out);
	pid_t cmdChld = fork();

	if(cmdChld > 0) {
		close(out[1]); // dont need to write to the pipe from this end.
		char cmdBuff[256];
		esend(clientsockfd, &SERVE_EXEC_BEGIN, sizeof(SERVE_EXEC_BEGIN), 0);

		int size = 0;
		while((size = read(out[0], &cmdBuff[0], sizeof(cmdBuff))) >0) {
			syslog(LOG_DEBUG, "cmd ouput: %i.", size);
			esend(socket, &cmdBuff[0], size, 0);
		}
		if(size == -1) {
			syslog(LOG_ERR, "Failed to read pipe!");
			syslog(LOG_ERR, "Error: %s", strerror(errno));
		}

		esend(clientsockfd, &SERVE_EXEC_END, sizeof(SERVE_EXEC_END), 0);
		close(out[0]);
	}
	else if(cmdChld == 0) {
		close(socket);
		close(out[0]); // dont need to read to the pipe from this end.

		// look at this again.
		// build the args array
		char ** args = NULL;
		char *p = strtok(cmd, " ");
		int numArgs = 0;

		while(p) {
			args = realloc (args, sizeof (char*) * ++numArgs);

			args[numArgs-1] = p;

			p = strtok (NULL, " ");
		}

		args = realloc (args, sizeof (char*) * (numArgs+1));
		args[numArgs] = 0;


		dup2(out[1], 1); //redirect our stdout to the pipe.

		if(execv(args[0], args) == -1) {
			syslog(LOG_ERR, "Failed to exec!");
			syslog(LOG_ERR, "Error: %s", strerror(errno));
			printf("Failed to run the server. Error: %s\n", strerror(errno)); // bit of a hack but it works.
		}
		else {
			syslog(LOG_DEBUG, "Exec complete!");
		}
		close(out[1]);
		exit(EXIT_FAILURE);
	}
	else if(cmdChld < 0) {
		syslog(LOG_ERR, "Failed to create fork!");
		esend(clientsockfd, &SERVE_BYE, sizeof(SERVE_BYE), 0);
		exit(EXIT_FAILURE);
	}

}

void termHandler(int sig) {

	if(!blockTerm) {
		syslog(LOG_DEBUG, "Signal handled!");
		if (clientsockfd > 0)
		{
			esend(clientsockfd, &SERVE_BYE, sizeof(SERVE_BYE), 0);
			close(clientsockfd);
		}
		if(sockfd > 0) {
			close(sockfd);
		}
		exit(EXIT_SUCCESS);
	}
	else {
		syslog(LOG_DEBUG, "Signal blocked!");
	}
}
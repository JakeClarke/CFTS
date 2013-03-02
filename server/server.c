#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>



int main(int argc, char *argv[]) 
{
	pid_t pid;
	char * configLoc = "~/.AOS.config";
	char * wkDir = "./";

	int sockfd, clientsockfd;
	struct sockaddr_in server;
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(4321);



	printf("hello server!\n");
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

	pid = fork();
	if(pid < 0) {
		syslog(LOG_ERR, "%s\n", perror);
		exit(EXIT_FAILURE);
	}

	if(pid > 0) /* get rid of the parent. */
		exit(EXIT_SUCCESS);

	syslog(LOG_INFO, "Server forked!");

	/* TODO change session and working directory */

	syslog(LOG_INFO, "Opening socket on port: %i", ntohs(server.sin_port));

	/* TODO open up socket */
}
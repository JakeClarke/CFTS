#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
	pid_t pid;
	char * configLoc = "~/.AOS.config";

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

	syslog(LOG_DEBUG, "Loading config: %s\n", configLoc);

	pid = fork();
	if(pid < 0) {
		syslog(LOG_ERR, "%s\n", perror);
		exit(EXIT_FAILURE);
	}

	if(pid > 0) 
		/* In the parent, let's bail */
		exit(EXIT_SUCCESS);

	syslog(LOG_INFO, "Server forked!");
}
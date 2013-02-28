#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int main() 
{
	pid_t pid;

	printf("hello server!\n");
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("AOS-server", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
	syslog(LOG_INFO, "Server started!");

	pid = fork();
    if(pid < 0) {
		syslog(LOG_ERR, "%s\n", perror);
		exit(EXIT_FAILURE);
    }

    if(pid > 0) 
		/* In the parent, let's bail */
		exit(EXIT_SUCCESS);
}
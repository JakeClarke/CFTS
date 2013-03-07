#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SIZE sizeof(struct sockaddr_in)

int main() 
{
	printf("AOS - client.\n");

	int sockfd;
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
		exit(1);
	}

	char * message = "BYE";
	send(sockfd, message, strlen(message), 0);
	printf("Shutting down!\n");
}
#ifndef ENC_H
#define ENC_H
#include <sys/socket.h>
#include <errno.h>

#define SALT "$1$w4acASuS"
#define MAGIC_NUMBER 142; // magic number for the xor function.

ssize_t esend(int sockfd, const void *buf, size_t len, int flags) {

	char ebuf[len]; // temp buffer, don't want to edit our source buffer.

	for(int i = 0; i < len; i++) {
		ebuf[i] = ((char *)buf)[i] ^ MAGIC_NUMBER; // xor with the magic number to get our encrypted form.
	}

	ssize_t rlen = send(sockfd, &ebuf[0], len, flags); // send.

	if(rlen < 0) {
		#ifndef SERVER
		printf("Failed to send!\n");
		printf("Error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
		#else 
		syslog(LOG_CRIT, "Failed to send!");
		syslog(LOG_CRIT, "Error: %s", strerror(errno));
		exit(EXIT_FAILURE);
		#endif
	}
	
	return rlen;
}

ssize_t drecv(int sockfd, void *buf, size_t len, int flags)  {
	ssize_t rlen = recv(sockfd, buf, len, flags);

	if(rlen < 0) {
		#ifndef SERVER
		printf("Failed to recv!\n");
		printf("Error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
		#else 
		syslog(LOG_CRIT, "Failed to recv!");
		syslog(LOG_CRIT, "Error: %s", strerror(errno));
		exit(EXIT_FAILURE);
		#endif
	}

	// not using a temp buffer because we can just modify data in place.
	for(int i = 0; i < rlen; i ++) {
		((char *)buf)[i] = ((char *)buf)[i] ^ MAGIC_NUMBER; // xor with the magic number get get our plain text form.
	}

	return rlen; // returrn the length of data recieved.
}

#endif
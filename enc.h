#ifndef ENC_H
#define ENC_H
#include <sys/socket.h>

#define MAGIC_NUMBER 142; // magic number for the xor function.

ssize_t esend(int sockfd, const void *buf, size_t len, int flags) {

	char ebuf[len]; // temp buffer, don't want to edit our source buffer.

	for(int i = 0; i < len; i++) {
		ebuf[i] = ((char *)buf)[i] ^ MAGIC_NUMBER; // xor with the magic number to get our encrypted form.
	}

	return send(sockfd, &ebuf[0], len, flags); // send.
}

ssize_t drecv(int sockfd, void *buf, size_t len, int flags)  {
	ssize_t rlen = recv(sockfd, buf, len, flags); 

	// not using a temp buffer because we can just modify data in place.
	for(int i = 0; i < rlen; i ++) {
		((char *)buf)[i] = ((char *)buf)[i] ^ MAGIC_NUMBER; // xor with the magic number get get our plain text form.
	}

	return rlen; // returrn the length of data recieved.
}

#endif
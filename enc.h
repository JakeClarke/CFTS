#ifndef ENC_H
#define ENC_H
#include <sys/socket.h>


const char MAGIC_NUMBER = 142;

ssize_t esend(int sockfd, const void *buf, size_t len, int flags) {

	char ebuf[len];

	for(int i = 0; i < len; i++) {
		ebuf[i] = ((char *)buf)[i] ^ MAGIC_NUMBER;
	}

	return send(sockfd, &ebuf, len, flags);
}

ssize_t drecv(int sockfd, void *buf, size_t len, int flags)  {
	ssize_t rlen = recv(sockfd, buf, len, flags);

	for(int i = 0; i < rlen; i ++) {
		((char *)buf)[i] = ((char *)buf)[i] ^ MAGIC_NUMBER;
	}

	return len;
}

#endif
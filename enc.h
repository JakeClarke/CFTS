#ifndef ENC_H
#define ENC_H
#include <sys/socket.h>


const int MAGIC_NUMBER = 1421;

ssize_t esend(int sockfd, const void *buf, size_t len, int flags) {

	char ebuf[len];

	for(int i = 0; i < len; i++) {
		ebuf[i] = buf[i] ^ MAGIC_NUMBER;
	}

	return send(sockfd, &ebuf, len, flags);
}

ssize_t drecv(int sockfd, void *buf, size_t len, int flags)  {
	ssize_t len = recv(sockfd, buf, len, flags);

	for(int i = 0; i < len; i ++) {
		buf[i] = buf[i] ^ MAGIC_NUMBER;
	}

	return len;
}

#endif
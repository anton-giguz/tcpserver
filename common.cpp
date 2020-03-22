/*
** common.cpp -- common functions for server and client
*/

#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

// write int32_t to socket, return false on error or when less than sizeof(int32_t) bytes sent
bool sendint32(int sockfd, int32_t value) {
    uint32_t h = 0;
    memcpy(&h, &value, sizeof h);
    uint32_t n = htonl(h);
    if (send(sockfd, &n, sizeof n, 0) != sizeof n) return false;
    return true;
}

// read int32_t from socket, return false on error or when less than sizeof(int32_t) bytes received
bool recvint32(int sockfd, int32_t *res) {
    uint32_t n = 0;
    if (recv(sockfd, &n, sizeof n, 0) != sizeof n) return false;
    uint32_t h = ntohl(n);
    memcpy(res, &h, sizeof *res);
    return true;
}

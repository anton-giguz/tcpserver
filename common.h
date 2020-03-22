#ifndef _COMMON_H
#define _COMMON_H

#include <cstdint>

extern bool sendint32(int sockfd, int32_t value);
extern bool recvint32(int sockfd, int32_t *res);

#endif

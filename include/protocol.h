#ifndef PROTOCOL_H
#define PROTOCOL
#include <stddef.h>

int slip_encode(int fd, const unsigned char* payload, size_t len);
int slip_decode(int fd, unsigned char* buf, size_t max_len);

#endif

#ifndef PROTOCOL_H
#define PROTOCOL
#include <stddef.h>
#include <stdint.h>

int slip_encode(int fd, const unsigned char* payload, size_t len);
int slip_decode(int fd, unsigned char* buf, size_t max_len);

int proto_sync(int fd);
int proto_spi_attach(int fd);
int proto_flash_begin(int fd, uint32_t total_size, uint32_t packet_count, uint32_t packet_size, uint32_t offset);
int proto_flash_data(int fd, const char* data, uint32_t len, uint32_t seq);
int proto_flash_end(int fd);

#endif

#ifndef SERIAL_H
#define SERIAL_H
#include <termios.h>
#include <unistd.h>

int serial_open(const char* device, speed_t baud); // returns fd or -1
int serial_close(int fd);

int serial_send_byte(int fd, unsigned char c);
int serial_recv_byte(int fd, unsigned char* out);
int serial_recv_byte_timeout(int fd, unsigned char* out, size_t timeout_ms);

#endif

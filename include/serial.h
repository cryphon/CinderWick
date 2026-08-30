#ifndef SERIAL_H
#define SERIAL_H
#include <termios.h>

int serial_open(const char* device, speed_t baud); // returns fd or -1
int serial_close(int fd);

int serial_send_byte(int fd, unsigned char c);
int serial_recv_byte(int fd, unsigned char* out);

#endif

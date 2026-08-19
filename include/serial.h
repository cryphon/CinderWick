#ifndef SERIAL_H
#define SERIAL_H
#include <termios.h>

int serial_open(const char* device, speed_t baud); // returns fd or -1
void serial_close(int fd);

#endif

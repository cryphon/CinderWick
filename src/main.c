#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "log.h"
#include "serial.h"
#include "reset.h"

void bridge(int tty_fd) {
    char c;
    fd_set readfds;

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(tty_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = (tty_fd > STDIN_FILENO ? tty_fd : STDIN_FILENO) + 1;

        int r = select(maxfd, &readfds, NULL, NULL, NULL); // NULL timeout = block indef.
        if(r < 0) {
            LOGE("select() failed");
            break;
        }

        if(FD_ISSET(tty_fd, &readfds)) {
            if(read(tty_fd, &c, 1) > 0) {
                write(STDOUT_FILENO, &c, 1);
                if(c == 'q') break; // exit cond. (serial side)
            }
        }

        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            if(read(STDIN_FILENO, &c, 1) > 0) {
                if(c == 'q') break;
                write(tty_fd, &c, 1);
            }
        }
    }
}

int main(void) {
    int fd = serial_open("/dev/ttyUSB0", B115200);
    if(fd < 0) {
        LOGE("Failed to open serial.\nTerminating...");
        return -1;
    }

    reset_to_wload(fd);
    bridge(fd);

    close(fd);
    return 0;
} 

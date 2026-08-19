#include "bridge.h"
#include "log.h"
#include <sys/select.h>
#include <unistd.h>


int interactive_bridge(int fd) {
    char c;
    fd_set readfds;

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = (fd > STDIN_FILENO ? fd : STDIN_FILENO) + 1;

        int r = select(maxfd, &readfds, NULL, NULL, NULL); // NULL timeout = block indef.
        if(r < 0) {
            LOGE("select() failed");
            break;
        }

        if(FD_ISSET(fd, &readfds)) {
            if(read(fd, &c, 1) > 0) {
                write(STDOUT_FILENO, &c, 1);
                if(c == 'q') break; // exit cond. (serial side)
            }
        }

        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            if(read(STDIN_FILENO, &c, 1) > 0) {
                if(c == 'q') break;
                write(fd, &c, 1);
            }
        }
    }
    return 0;
}

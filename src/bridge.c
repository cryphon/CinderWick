#include "bridge.h"
#include "log.h"
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


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
            ssize_t n = read(fd, &c, 1);
            if(n < 0) {
                if(errno == EINTR) continue;
                LOGE("read(serial) failed: %s", strerror(errno));
                return -1;
            }
            if(n == 0) {
                LOGE("serial device closed unexpectedly");
                return -1;
            }
            if(write(STDOUT_FILENO, &c, 1) != 1) {
                LOGE("write(stdout failed: %s", strerror(errno));
                return -1;
            }
            if(c == 'q') break;
        }

        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if(n < 0) {
                if(errno == EINTR) continue;
                LOGE("read(stdin) failed: %s", strerror(errno));
                return -1;
            }
            if(n == 0) break; //stdin closed (piped input ended)
            if(c == 'q') break;
            if(write(fd, &c, 1) != 1) {
                LOGE("write(serial) failed: %s", strerror(errno));
                return -1;
            }
        }
    }
    return 0;
}

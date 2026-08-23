#include "reset.h"
#include "log.h"
#include <sys/ioctl.h>
#include <unistd.h>


void print_line_state(int fd, const char* label) {
    int status;
    ioctl(fd, TIOCMGET, &status);
    LOGI("%s: RTS=%d DTR=%d", label,
         (status & TIOCM_RTS) ? 1 : 0,
         (status & TIOCM_DTR) ? 1 : 0);
}

int set_rts_dtr_lines(int fd, int rts, int dtr) {
    int stat;
    ssize_t res;
    
    res = ioctl(fd, TIOCMGET, &stat);
    if(res < 0) {
        LOGE("Failed to get status of the modem bits");
        return -1;
    }

    if(rts) stat |= TIOCM_RTS; else stat &= ~TIOCM_RTS;
    if(dtr) stat |= TIOCM_DTR; else stat &= ~TIOCM_DTR;
    res = ioctl(fd, TIOCMSET, &stat); // single atomic(ish) call
    if(res < 0) {
        LOGE("Failed to set status of modem bits");
        return -1;
    }
    return 0;
}


int reset_to_wload(int fd) {
    ssize_t res;
    // 1. Assert EN (RTS) -> chip held in reset
    if(set_rts_dtr_lines(fd, 1, 0) < 0) {
        LOGE("reset_to_wload: Failed to assert EN");
        return -1;
    }
    print_line_state(fd, "assert EN");
    usleep(50000);
    
    // 2. Release EN (RTS) FIRST -> chip exits reset, samples GPIO0 = LOW -> download
    if(set_rts_dtr_lines(fd, 0, 1) < 0) {
        LOGE("reset_to_wload: Failed to release EN");
        return -1;
    }
    print_line_state(fd, "release EN");
    usleep(50000); // Wait for bootstrap sampling

    // 3. Release GPIO0 (DTR) LAST -> back to HIGH
    if(set_rts_dtr_lines(fd, 0, 0) < 0) {
        LOGE("reset_to_wload: Failed to release GPIO0");
        return -1;
    }
    print_line_state(fd, "release DTR");
    return 0;
}

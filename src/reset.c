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

void set_rts_dtr_lines(int fd, int rts, int dtr) {
    int stat;
    ioctl(fd, TIOCMGET, &stat);
    if(rts) stat |= TIOCM_RTS; else stat &= ~TIOCM_RTS;
    if(dtr) stat |= TIOCM_DTR; else stat &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &stat); // single atomic(ish) call
}


int reset_to_wload(int fd) {
    // 1. Assert EN (RTS) -> chip held in reset
    set_rts_dtr_lines(fd, 1, 0);
    print_line_state(fd, "assert EN");
    usleep(50000);
    
    // 2. Release EN (RTS) FIRST -> chip exits reset, samples GPIO0 = LOW -> download
    set_rts_dtr_lines(fd, 0, 1);
    print_line_state(fd, "release EN");
    usleep(50000); // Wait for bootstrap sampling

    // 3. Release GPIO0 (DTR) LAST -> back to HIGH
    set_rts_dtr_lines(fd, 0, 0);
    print_line_state(fd, "release DTR");
}

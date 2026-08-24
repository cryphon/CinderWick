#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "log.h"
#include "serial.h"
#include "reset.h"
#include "bridge.h"

int main(void) {
    int fd = serial_open("/dev/ttyUSB0", B115200);
    if(fd < 0) {
        LOGE("Failed to open serial.\nTerminating...");
        return -1;
    }

    if(reset_to_wload(fd) < 0) {
        LOGE("Failed to reset chip into DOWNLOAD");
        return -1;
    }

    if(interactive_bridge(fd) < 0) {
        LOGE("Interactive bridge failed");
        return -1;
    }

    serial_close(fd);
    return 0;
} 

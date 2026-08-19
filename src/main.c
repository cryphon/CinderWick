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

    reset_to_wload(fd);
    interactive_bridge(fd);

    serial_close(fd);
    return 0;
} 

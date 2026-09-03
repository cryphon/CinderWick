#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <assert.h>

#include "log.h"
#include "serial.h"
#include "reset.h"
#include "bridge.h"
#include "protocol.h"


#define BRIDGE_BIT 0

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

    if(BRIDGE_BIT) {
        if(interactive_bridge(fd) < 0) {
            LOGE("Interactive bridge failed");
            return -1;
        }
    }
    tcflush(fd, TCIFLUSH);

    if(proto_sync(fd) < 0) { LOGE("sync failed"); return -1; }
    if(proto_spi_attach(fd) < 0) { LOGE("spi_attach failed"); return -1; }



    serial_close(fd);
    return 0;
} 

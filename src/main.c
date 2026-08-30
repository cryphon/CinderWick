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

    unsigned char sync_cmd[] = {
        0x00, 0x08, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,  // direction, opcode, len(0x24,0x00), checksum(0x00000000)
        0x07, 0x07, 0x12, 0x20,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55
    }; 
    tcflush(fd, TCIFLUSH);
    int synced = 0;
    for (int attempt = 0; attempt < 7 && !synced; attempt++) {
        slip_encode(fd, sync_cmd, sizeof(sync_cmd));
        tcdrain(fd);

        unsigned char decoded[64];
        int len = slip_decode(fd, decoded, sizeof(decoded)); // 100ms per attempt
        if (len > 0) {
            LOGI("Sync succeeded on attempt %d, got %d bytes:", attempt + 1, len);
            for (int i = 0; i < len; i++) fprintf(stderr, "%02x ", decoded[i]);
            fprintf(stderr, "\n");
            synced = 1;
        } else {
            LOGW("Sync attempt %d failed, retrying...", attempt + 1);
        }
    }
    if (!synced) LOGE("Sync failed after all attempts");
    serial_close(fd);
    return 0;
} 

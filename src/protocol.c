#include "protocol.h"
#include "serial.h"
#include "log.h"

#define END         0300    /* End of packet */
#define ESC         0333    /* Byte stuffing */
#define ESC_END     0334    /* ESC ESC_END means END data byte */
#define ESC_ESC     0335    /* ESC ESC_ESC means ESC data byte */
#define SERIAL_TIMEOUT 100    /* In MS */

int slip_encode(int fd, const unsigned char* payload, size_t len) {
    serial_send_byte(fd, END);

    for(size_t i = 0; i < len; i++) {
        if(payload[i] == END) {
            serial_send_byte(fd, ESC);
            serial_send_byte(fd, ESC_END);
        }
        else if(payload[i] == ESC) {
            serial_send_byte(fd, ESC);
            serial_send_byte(fd, ESC_ESC);
        }
        else {
            serial_send_byte(fd, payload[i]);
        }
    }
    serial_send_byte(fd, END);

    return 0;
}

int slip_decode(int fd, unsigned char* buf, size_t max_len) {
    unsigned char c;
    size_t out_len = 0;

    /* Wait for leading END byte that starts frame. */
    if(serial_recv_byte_timeout(fd, &c, SERIAL_TIMEOUT) < 0) return -1;
    if(c != END) {
        LOGE("Expected SLIP frame start (0XC0) got 0x%02x", c);
        return -1;
    }

    while(1) {
        if(serial_recv_byte_timeout(fd, &c, SERIAL_TIMEOUT) < 0) return -1;
        if(c == END) {
            // Frame end
            return (int)out_len;
        }
        else if (c == ESC) {
            // Escaped byte follows
            unsigned char esc;
            if(serial_recv_byte_timeout(fd, &esc, SERIAL_TIMEOUT) < 0) return -1;
            
            if(esc == ESC_END) {
                c = END;
            }
            else if( esc == ESC_ESC) {
                c = ESC;
            }
            else {
                LOGE("Invalid SLIP escape sequence: -xDB -x%02x", esc);
                return -1;
            }
        }
        // else: c is a leteral data byte, store

        if(out_len >= max_len) {
            LOGE("slip_decode: output buffer full, frame too large");
            return -1;
        }
        buf[out_len++] = c;
    }
    return 0;
}

// Sends cmd (already full frame body, unencoded), retries on timeout/failure
// and returns the decoded response length, or -1 if all attempts fail
static int proto_send_and_wait(int fd, 
        const unsigned char* cmd, size_t cmd_len,
        unsigned char* resp_buf, size_t resp_buf_len,
        int max_attempts, const char* lbl) {
    for(int attempt = 0; attempt < max_attempts; attempt++) {
        tcflush(fd, TCIFLUSH);

        slip_encode(fd, cmd, cmd_len);
        if(tcdrain(fd) < 0) {
            LOGE("%s: tcdrain failed on attempt %d", lbl, attempt + 1);
            continue;
        }

        int len = slip_decode(fd, resp_buf, resp_buf_len);
        if(len > 0) {
            LOGI("%s: succeeded on attempt %d (%d bytes)", lbl, attempt + 1, len);
            return len;
        }

        LOGW("%s: attempt %d failed, retrying...", lbl, attempt + 1);
    }

    LOGE("%s: failed after %d attempts", lbl, max_attempts);
    return -1;
}


int proto_sync(int fd) {
    unsigned char sync_cmd[] = {
        0x00, 0x08, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,  // direction, opcode, len(0x24,0x00), checksum(0x00000000)
        0x07, 0x07, 0x12, 0x20,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55
    }; 

    unsigned char response[64];
    int len = proto_send_and_wait(fd, sync_cmd, sizeof(sync_cmd), 
            response, sizeof(response), 7, "SYNC");

    if(len < 0) return -1;

    if(len < 4 || response[0] != 0x01 || response[1] != 0x08) {
        LOGE("SYNC: unexpected response header");
        return -1;
    }
    return 0;
}

int proto_spi_attach(int fd) {
    unsigned char spi_attach_cmd[] = {
        0x00, 0x0D, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, // spi_config = 0 (use default pins)
        0x00, 0x00, 0x00, 0x00 // reserved
    };

    unsigned char response[10];
    int len = proto_send_and_wait(fd, spi_attach_cmd, sizeof(spi_attach_cmd),
            response, sizeof(response), 7, "SPI_ATTACH");

    if(len < 0) return -1;
    if(len < 4 || response[0] != 0x01 || response[1] != 0x0D) {
        LOGE("SPI_ATTACH: unexpected response header");
        return -1;
    }
    return 0;
}
